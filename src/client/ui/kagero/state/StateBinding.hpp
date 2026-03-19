#pragma once

#include "StateStore.hpp"
#include "ReactiveState.hpp"
#include <functional>

namespace mc::client::ui::kagero::state {

/**
 * @brief 状态绑定工具
 *
 * 提供状态与组件之间的绑定功能
 */
namespace binding {

/**
 * @brief 创建状态绑定
 *
 * @tparam T 状态类型
 * @param key 状态键
 * @return 绑定点
 */
template<typename T>
StateBindingPoint bind(const String& key) {
    return StateBindingPoint(key);
}

/**
 * @brief 双向绑定到 Reactive 状态
 *
 * @tparam T 状态类型
 * @param reactive 响应式状态引用
 * @param key 存储键
 * @return 绑定点
 */
template<typename T>
StateBindingPoint bindReactive(Reactive<T>& reactive, const String& key) {
    // 创建双向同步
    StateStore::instance().set(key, reactive.get());

    // 从 Reactive 到 Store
    reactive.observe([&key](const T& oldValue, const T& newValue) {
        StateStore::instance().set(key, newValue);
    });

    // 从 Store 到 Reactive
    StateStore::instance().subscribe(key, [&reactive]() {
        T value = StateStore::instance().get<T>(key);
        if (reactive.get() != value) {
            reactive.set(value);
        }
    });

    return StateBindingPoint(key);
}

/**
 * @brief 创建计算属性
 *
 * @tparam T 结果类型
 * @tparam Func 计算函数类型
 * @param compute 计算函数
 * @return 计算属性
 */
template<typename T, typename Func>
Computed<T> computed(Func&& compute) {
    return Computed<T>(std::forward<Func>(compute));
}

/**
 * @brief 监视状态变化
 *
 * @tparam T 状态类型
 * @param key 状态键
 * @param callback 回调函数
 * @return 订阅ID
 */
template<typename T>
u64 watch(const String& key, std::function<void(const T&, const T&)> callback) {
    T oldValue = StateStore::instance().get<T>(key);
    return StateStore::instance().subscribe(key, [key, callback, oldValue]() mutable {
        T newValue = StateStore::instance().get<T>(key);
        if (oldValue != newValue) {
            callback(oldValue, newValue);
            oldValue = newValue;
        }
    });
}

/**
 * @brief 监视多个状态变化
 *
 * @param keys 状态键列表
 * @param callback 回调函数
 * @return 订阅ID列表
 */
inline std::vector<u64> watchAll(const std::vector<String>& keys, std::function<void()> callback) {
    std::vector<u64> ids;
    ids.reserve(keys.size());
    for (const auto& key : keys) {
        ids.push_back(StateStore::instance().subscribe(key, callback));
    }
    return ids;
}

/**
 * @brief 取消监视
 */
inline void unwatch(u64 id) {
    StateStore::instance().unsubscribe(id);
}

/**
 * @brief 取消多个监视
 */
inline void unwatchAll(const std::vector<u64>& ids) {
    for (u64 id : ids) {
        StateStore::instance().unsubscribe(id);
    }
}

} // namespace binding

/**
 * @brief 状态作用域
 *
 * 用于管理一组相关的状态绑定
 */
class StateScope {
public:
    StateScope() = default;
    ~StateScope() {
        for (u64 id : m_subscriptions) {
            StateStore::instance().unsubscribe(id);
        }
    }

    // 禁止拷贝
    StateScope(const StateScope&) = delete;
    StateScope& operator=(const StateScope&) = delete;

    // 允许移动
    StateScope(StateScope&& other) noexcept
        : m_subscriptions(std::move(other.m_subscriptions)) {}

    StateScope& operator=(StateScope&& other) noexcept {
        if (this != &other) {
            for (u64 id : m_subscriptions) {
                StateStore::instance().unsubscribe(id);
            }
            m_subscriptions = std::move(other.m_subscriptions);
        }
        return *this;
    }

    /**
     * @brief 订阅状态变化
     */
    u64 subscribe(const String& key, std::function<void()> callback) {
        u64 id = StateStore::instance().subscribe(key, std::move(callback));
        m_subscriptions.push_back(id);
        return id;
    }

    /**
     * @brief 监视状态变化
     */
    template<typename T>
    u64 watch(const String& key, std::function<void(const T&, const T&)> callback) {
        u64 id = binding::watch<T>(key, std::move(callback));
        m_subscriptions.push_back(id);
        return id;
    }

    /**
     * @brief 取消所有订阅
     */
    void clear() {
        for (u64 id : m_subscriptions) {
            StateStore::instance().unsubscribe(id);
        }
        m_subscriptions.clear();
    }

    /**
     * @brief 获取订阅数量
     */
    [[nodiscard]] size_t size() const { return m_subscriptions.size(); }

private:
    std::vector<u64> m_subscriptions;
};

/**
 * @brief 状态上下文
 *
 * 提供组件级别的状态管理
 */
class StateContext {
public:
    StateContext() = default;

    /**
     * @brief 获取状态值
     */
    template<typename T>
    [[nodiscard]] T get(const String& key) const {
        return StateStore::instance().get<T>(key);
    }

    /**
     * @brief 设置状态值
     */
    template<typename T>
    void set(const String& key, T value) {
        StateStore::instance().set(key, std::move(value));
    }

    /**
     * @brief 创建响应式状态
     */
    template<typename T>
    Reactive<T>& reactive(const String& key, T initialValue = T{}) {
        auto it = m_reactives.find(key);
        if (it != m_reactives.end()) {
            return *static_cast<Reactive<T>*>(it->second.get());
        }

        auto reactive = std::make_unique<Reactive<T>>(std::move(initialValue));
        auto* ptr = reactive.get();
        m_reactives[key] = std::move(reactive);
        return *ptr;
    }

    /**
     * @brief 获取状态作用域
     */
    StateScope& scope() { return m_scope; }

private:
    std::unordered_map<String, std::unique_ptr<void, void(*)(void*)>> m_reactives;
    StateScope m_scope;
};

} // namespace mc::client::ui::kagero::state
