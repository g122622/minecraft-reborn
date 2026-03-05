#pragma once

#include "IProperty.hpp"
#include "StateHolder.hpp"
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
 * 用法示例:
 * @code
 * // 使用Builder构建状态容器
 * auto container = StateContainer<Block, BlockState>::Builder(block)
 *     .add(BlockStateProperties::FACING())
 *     .add(BlockStateProperties::LIT())
 *     .create([](const Block& block, auto values, u32 id) {
 *         return std::make_unique<BlockState>(block, values, id);
 *     });
 *
 * // 获取基础状态
 * const BlockState& baseState = container->baseState();
 *
 * // 获取所有有效状态
 * for (const auto& state : container->validStates()) {
 *     // ...
 * }
 *
 * // 根据ID获取状态
 * const BlockState* state = container->getStateById(5);
 * @endcode
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
        /**
         * @brief 构造构建器
         * @param owner 拥有者引用
         */
        explicit Builder(Owner& owner)
            : m_owner(owner) {
        }

        /**
         * @brief 添加属性
         * @param prop 属性指针
         * @return 构建器引用
         */
        Builder& add(const IProperty* prop) {
            validateProperty(*prop);
            m_properties[prop->name()] = prop;
            return *this;
        }

        /**
         * @brief 添加布尔属性
         * @param name 属性名称
         * @return 构建器引用
         */
        Builder& addBoolean(const String& name) {
            auto prop = BooleanProperty::create(name);
            return add(prop.release());
        }

        /**
         * @brief 添加整数属性
         * @param name 属性名称
         * @param min 最小值
         * @param max 最大值
         * @return 构建器引用
         */
        Builder& addInteger(const String& name, i32 min, i32 max) {
            auto prop = IntegerProperty::create(name, min, max);
            return add(prop.release());
        }

        /**
         * @brief 添加方向属性（所有方向）
         * @param name 属性名称
         * @return 构建器引用
         */
        Builder& addDirection(const String& name) {
            auto prop = DirectionProperty::create(name);
            return add(prop.release());
        }

        /**
         * @brief 添加方向属性（仅水平方向）
         * @param name 属性名称
         * @return 构建器引用
         */
        Builder& addHorizontalDirection(const String& name) {
            auto prop = DirectionProperty::createHorizontal(name);
            return add(prop.release());
        }

        /**
         * @brief 添加坐标轴属性
         * @param name 属性名称
         * @return 构建器引用
         */
        Builder& addAxis(const String& name) {
            auto prop = EnumProperty<Axis>::createAll(name);
            return add(prop.release());
        }

        /**
         * @brief 构建状态容器
         * @param factory 状态工厂函数
         * @return 状态容器
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
            // 验证属性名称格式
            static const std::regex NAME_PATTERN("^[a-z0-9_]+$");
            if (!std::regex_match(prop.name(), NAME_PATTERN)) {
                throw std::invalid_argument(
                    "Invalid property name: " + prop.name()
                );
            }

            // 验证属性值数量
            if (prop.valueCount() <= 1) {
                throw std::invalid_argument(
                    "Property " + prop.name() + " must have more than 1 possible value"
                );
            }

            // 验证所有值名称格式
            for (size_t i = 0; i < prop.valueCount(); ++i) {
                if (!std::regex_match(prop.valueToString(i), NAME_PATTERN)) {
                    throw std::invalid_argument(
                        "Property " + prop.name() + " has invalid value name: " + prop.valueToString(i)
                    );
                }
            }

            // 检查重复
            if (m_properties.find(prop.name()) != m_properties.end()) {
                throw std::invalid_argument(
                    "Duplicate property: " + prop.name()
                );
            }
        }
    };

    /**
     * @brief 获取基础状态（第一个状态）
     */
    [[nodiscard]] const State& baseState() const {
        return *m_states[0];
    }

    /**
     * @brief 获取所有有效状态
     */
    [[nodiscard]] const std::vector<std::unique_ptr<State>>& validStates() const {
        return m_states;
    }

    /**
     * @brief 获取状态数量
     */
    [[nodiscard]] size_t stateCount() const {
        return m_states.size();
    }

    /**
     * @brief 获取拥有者
     */
    [[nodiscard]] const Owner& owner() const {
        return m_owner;
    }

    /**
     * @brief 获取所有属性
     */
    [[nodiscard]] const std::unordered_map<String, std::unique_ptr<IProperty>>& properties() const {
        return m_properties;
    }

    /**
     * @brief 根据名称获取属性
     * @param name 属性名称
     * @return 属性指针，不存在返回nullptr
     */
    [[nodiscard]] const IProperty* getProperty(StringView name) const {
        auto it = m_properties.find(String(name));
        return it != m_properties.end() ? it->second.get() : nullptr;
    }

    /**
     * @brief 根据ID获取状态
     * @param id 状态ID
     * @return 状态指针，不存在返回nullptr
     */
    [[nodiscard]] const State* getStateById(u32 id) const {
        if (id >= m_states.size()) {
            return nullptr;
        }
        return m_states[id].get();
    }

    /**
     * @brief 转换为字符串表示
     */
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
        // 复制属性到内部存储
        for (auto& [name, prop] : propertiesToTransfer) {
            m_properties[name] = std::unique_ptr<IProperty>(const_cast<IProperty*>(prop));
        }

        // 收集属性指针用于状态生成
        std::vector<const IProperty*> props;
        for (const auto& [name, prop] : m_properties) {
            props.push_back(prop.get());
        }

        // 生成所有状态组合
        generateStates(props, factory);
    }

    void generateStates(const std::vector<const IProperty*>& props, StateFactory factory) {
        // 计算总状态数
        size_t totalStates = 1;
        for (const auto* prop : props) {
            totalStates *= prop->valueCount();
        }

        m_states.reserve(totalStates);

        // 生成所有组合
        std::vector<size_t> indices(props.size(), 0);

        std::unordered_map<const IProperty*, size_t> values;
        for (const auto* prop : props) {
            values[prop] = 0;
        }

        // 状态映射（用于建立转换表）
        std::vector<std::pair<std::unordered_map<const IProperty*, size_t>, State*>> stateMap;
        stateMap.reserve(totalStates);

        u32 stateId = 0;
        while (true) {
            // 创建当前组合的状态
            auto state = factory(m_owner, values, stateId);
            stateMap.emplace_back(values, state.get());
            m_states.push_back(std::move(state));
            stateId++;

            // 生成下一个组合
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

            if (propIndex >= props.size()) {
                break;
            }
        }

        // 建立转换表
        for (auto& [vals, state] : stateMap) {
            std::unordered_map<const IProperty*, std::vector<const State*>> transitions;

            for (const auto* prop : props) {
                std::vector<const State*> valueTransitions(prop->valueCount(), nullptr);

                for (size_t i = 0; i < prop->valueCount(); ++i) {
                    // 复制当前值并修改
                    auto targetValues = vals;
                    targetValues[prop] = i;

                    // 查找目标状态
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

            // 初始化状态的转换表
            state->initTransitions(std::move(transitions));
        }

        // 建立ID映射
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
