#pragma once

#include "IProperty.hpp"
#include "StateHolder.hpp"
#include "Property.hpp"
#include "BooleanProperty.hpp"
#include "IntegerProperty.hpp"
#include "DirectionProperty.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <regex>

namespace mr {

/**
 * @brief 状态容器模板
 *
 * 预计算并管理所有可能的状态组合。
 *
 * 参考: net.minecraft.state.StateContainer<O, S>
 *
 * @tparam Owner 拥有者类型
 * @tparam State 状态类型
 *
 * 注意:
 * - 状态数量 = 各属性值数量的乘积，可能非常大
 * - 属性名称必须符合 [a-z0-9_]+ 格式
 * - 每个属性至少需要2个值
 */
template<typename Owner, typename State>
class StateContainer {
public:
    using StateFactory = std::function<std::unique_ptr<State>(
        const Owner&, std::unordered_map<const IProperty*, size_t>, u32)>;

    /**
     * @brief 构建器
     */
    class Builder {
    public:
        explicit Builder(Owner& owner) : m_owner(owner) {}

        /**
         * @brief 添加属性（指针版本）
         */
        Builder& add(const IProperty* prop) {
            validateProperty(*prop);
            m_properties[prop->name()] = prop;
            return *this;
        }

        /**
         * @brief 添加属性（引用版本）
         */
        template<typename T>
        Builder& add(const Property<T>& prop) {
            return add(static_cast<const IProperty*>(&prop));
        }

        /**
         * @brief 添加布尔属性
         */
        Builder& addBoolean(const String& name) {
            auto prop = BooleanProperty::create(name);
            return add(prop.release());
        }

        /**
         * @brief 添加整数属性
         */
        Builder& addInteger(const String& name, i32 min, i32 max) {
            auto prop = IntegerProperty::create(name, min, max);
            return add(prop.release());
        }

        /**
         * @brief 添加方向属性（所有方向）
         */
        Builder& addDirection(const String& name) {
            auto prop = DirectionProperty::create(name);
            return add(prop.release());
        }

        /**
         * @brief 添加方向属性（仅水平方向）
         */
        Builder& addHorizontalDirection(const String& name) {
            auto prop = DirectionProperty::createHorizontal(name);
            return add(prop.release());
        }

        /**
         * @brief 添加坐标轴属性
         */
        Builder& addAxis(const String& name) {
            auto prop = AxisProperty::create(name);
            return add(prop.release());
        }

        /**
         * @brief 构建状态容器
         */
        std::unique_ptr<StateContainer> create(StateFactory factory) {
            return std::unique_ptr<StateContainer>(new StateContainer(
                m_owner, std::move(m_properties), factory
            ));
        }

    private:
        Owner& m_owner;
        std::unordered_map<String, const IProperty*> m_properties;

        void validateProperty(const IProperty& prop) {
            static const std::regex NAME_PATTERN("^[a-z0-9_]+$");
            if (!std::regex_match(prop.name(), NAME_PATTERN)) {
                throw std::invalid_argument("Invalid property name: " + prop.name());
            }
            if (prop.valueCount() <= 1) {
                throw std::invalid_argument(
                    "Property " + prop.name() + " must have more than 1 possible value"
                );
            }
            for (size_t i = 0; i < prop.valueCount(); ++i) {
                if (!std::regex_match(prop.valueToString(i), NAME_PATTERN)) {
                    throw std::invalid_argument(
                        "Property " + prop.name() + " has invalid value name: " + prop.valueToString(i)
                    );
                }
            }
            if (m_properties.find(prop.name()) != m_properties.end()) {
                throw std::invalid_argument("Duplicate property: " + prop.name());
            }
        }
    };

    [[nodiscard]] const State& baseState() const { return *m_states[0]; }
    [[nodiscard]] const std::vector<std::unique_ptr<State>>& validStates() const { return m_states; }
    [[nodiscard]] size_t stateCount() const { return m_states.size(); }
    [[nodiscard]] const Owner& owner() const { return m_owner; }
    [[nodiscard]] const std::unordered_map<String, std::unique_ptr<IProperty>>& properties() const { return m_properties; }

    [[nodiscard]] const IProperty* getProperty(StringView name) const {
        auto it = m_properties.find(String(name));
        return it != m_properties.end() ? it->second.get() : nullptr;
    }

    [[nodiscard]] const State* getStateById(u32 id) const {
        if (id >= m_states.size()) return nullptr;
        return m_states[id].get();
    }

    [[nodiscard]] String toString() const {
        std::ostringstream ss;
        ss << "StateContainer{owner=" << typeid(Owner).name();
        if (!m_properties.empty()) {
            ss << ", properties=[";
            bool first = true;
            for (const auto& [name, prop] : m_properties) {
                if (!first) ss << ", ";
                ss << name;
                first = false;
            }
            ss << "]";
        }
        ss << ", states=" << m_states.size() << "}";
        return ss.str();
    }

private:
    StateContainer(Owner& owner,
                   std::unordered_map<String, const IProperty*> propertiesToTransfer,
                   StateFactory factory)
        : m_owner(owner) {
        for (auto& [name, prop] : propertiesToTransfer) {
            m_properties[name] = std::unique_ptr<IProperty>(const_cast<IProperty*>(prop));
        }
        std::vector<const IProperty*> props;
        for (const auto& [name, prop] : m_properties) {
            props.push_back(prop.get());
        }
        generateStates(props, factory);
    }

    void generateStates(const std::vector<const IProperty*>& props, StateFactory factory) {
        size_t totalStates = 1;
        for (const auto* prop : props) {
            totalStates *= prop->valueCount();
        }
        m_states.reserve(totalStates);
        std::vector<size_t> indices(props.size(), 0);
        std::unordered_map<const IProperty*, size_t> values;
        for (const auto* prop : props) {
            values[prop] = 0;
        }
        std::vector<std::pair<std::unordered_map<const IProperty*, size_t>, State*>> stateMap;
        stateMap.reserve(totalStates);

        u32 stateId = 0;
        while (true) {
            auto state = factory(m_owner, values, stateId);
            stateMap.emplace_back(values, state.get());
            m_states.push_back(std::move(state));
            stateId++;
            size_t propIndex = 0;
            while (propIndex < props.size()) {
                indices[propIndex]++;
                if (indices[propIndex] < props[propIndex]->valueCount()) {
                    values[props[propIndex]] = indices[propIndex];
                    break;
                }
                indices[propIndex] = 0;
                values[props[propIndex]] = 0;
                propIndex++;
            }
            if (propIndex >= props.size()) break;
        }

        for (auto& [vals, state] : stateMap) {
            std::unordered_map<const IProperty*, std::vector<const State*>> transitions;
            for (const auto* prop : props) {
                std::vector<const State*> valueTransitions(prop->valueCount(), nullptr);
                for (size_t i = 0; i < prop->valueCount(); ++i) {
                    auto targetValues = vals;
                    targetValues[prop] = i;
                    for (auto& [targetVals, targetState] : stateMap) {
                        bool match = true;
                        for (const auto* p : props) {
                            if (targetValues[p] != targetVals[p]) {
                                match = false;
                                break;
                            }
                        }
                        if (match) {
                            valueTransitions[i] = targetState;
                            break;
                        }
                    }
                }
                transitions[prop] = std::move(valueTransitions);
            }
            state->initTransitions(std::move(transitions));
        }
        for (const auto& state : m_states) {
            m_stateIdMap[state->stateId()] = state.get();
        }
    }

    Owner& m_owner;
    std::unordered_map<String, std::unique_ptr<IProperty>> m_properties;
    std::vector<std::unique_ptr<State>> m_states;
    std::unordered_map<u32, State*> m_stateIdMap;
};

} // namespace mr
