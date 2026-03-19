#pragma once

#include "../Types.hpp"
#include <any>
#include <functional>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <atomic>

namespace mc::client::ui::kagero::state {

/**
 * @brief 状态存储
 *
 * 全局状态管理器，支持：
 * - 键值对存储任意类型值
 * - 订阅状态变化
 * - 动作分发（类似Redux）
 *
 * 使用示例：
 * @code
 * StateStore& store = StateStore::instance();
 *
 * // 设置状态
 * store.set("player.health", 100);
 * store.set("player.maxHealth", 100);
 *
 * // 获取状态
 * i32 health = store.get<i32>("player.health");
 *
 * // 订阅变化
 * u64 id = store.subscribe("player.health", []() {
 *     std::cout << "Health changed!" << std::endl;
 * });
 *
 * // 取消订阅
 * store.unsubscribe(id);
 * @endcode
 */
class StateStore {
public:
    using Subscriber = std::function<void()>;
    using SubscriberId = u64;
    using Action = std::function<void(StateStore&)>;
    using Middleware = std::function<void(const String&, const std::any&, StateStore&)>;

    /**
     * @brief 获取单例实例
     */
    static StateStore& instance() {
        static StateStore instance;
        return instance;
    }

    // 禁止拷贝和移动
    StateStore(const StateStore&) = delete;
    StateStore& operator=(const StateStore&) = delete;

    // ==================== 状态访问 ====================

    /**
     * @brief 获取状态值
     *
     * @tparam T 值类型
     * @param key 状态键
     * @param defaultValue 默认值（如果键不存在）
     * @return 状态值
     */
    template<typename T>
    [[nodiscard]] T get(const String& key, const T& defaultValue = T{}) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_state.find(key);
        if (it != m_state.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (const std::bad_any_cast&) {
                return defaultValue;
            }
        }
        return defaultValue;
    }

    /**
     * @brief 检查键是否存在
     */
    [[nodiscard]] bool has(const String& key) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_state.find(key) != m_state.end();
    }

    /**
     * @brief 设置状态值
     *
     * @tparam T 值类型
     * @param key 状态键
     * @param value 新值
     */
    template<typename T>
    void set(const String& key, T value) {
        std::vector<Subscriber> subscribersToNotify;

        {
            std::lock_guard<std::mutex> lock(m_mutex);

            // 调用中间件
            for (const auto& middleware : m_middlewares) {
                middleware(key, value, *this);
            }

            // 设置值
            m_state[key] = std::any(std::move(value));

            // 收集需要通知的订阅者
            auto it = m_subscribers.find(key);
            if (it != m_subscribers.end()) {
                for (const auto& pair : it->second) {
                    subscribersToNotify.push_back(pair.second);
                }
            }
        }

        // 在锁外通知订阅者
        for (const auto& subscriber : subscribersToNotify) {
            subscriber();
        }
    }

    /**
     * @brief 删除状态值
     */
    void remove(const String& key) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_state.erase(key);
    }

    /**
     * @brief 清空所有状态
     */
    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_state.clear();
    }

    /**
     * @brief 获取所有键
     */
    [[nodiscard]] std::vector<String> keys() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<String> result;
        result.reserve(m_state.size());
        for (const auto& pair : m_state) {
            result.push_back(pair.first);
        }
        return result;
    }

    // ==================== 订阅 ====================

    /**
     * @brief 订阅状态变化
     *
     * @param key 状态键
     * @param subscriber 订阅者函数
     * @return 订阅者ID，用于取消订阅
     */
    SubscriberId subscribe(const String& key, Subscriber subscriber) {
        std::lock_guard<std::mutex> lock(m_mutex);
        SubscriberId id = m_nextSubscriberId++;
        m_subscribers[key].emplace_back(id, std::move(subscriber));
        m_subscriberToKey[id] = key;
        return id;
    }

    /**
     * @brief 取消订阅
     *
     * @param id 订阅者ID
     * @return 是否成功取消
     */
    bool unsubscribe(SubscriberId id) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto keyIt = m_subscriberToKey.find(id);
        if (keyIt == m_subscriberToKey.end()) {
            return false;
        }

        const String& key = keyIt->second;
        auto& subscribers = m_subscribers[key];
        auto it = std::find_if(subscribers.begin(), subscribers.end(),
            [id](const auto& pair) { return pair.first == id; });

        if (it != subscribers.end()) {
            subscribers.erase(it);
            m_subscriberToKey.erase(keyIt);
            return true;
        }

        return false;
    }

    /**
     * @brief 取消某键的所有订阅
     */
    void unsubscribeAll(const String& key) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_subscribers.find(key);
        if (it != m_subscribers.end()) {
            for (const auto& pair : it->second) {
                m_subscriberToKey.erase(pair.first);
            }
            m_subscribers.erase(it);
        }
    }

    // ==================== 动作 ====================

    /**
     * @brief 派发动作
     *
     * @param action 动作函数
     */
    void dispatch(Action action) {
        action(*this);
    }

    /**
     * @brief 添加中间件
     *
     * 中间件在每次状态变化时被调用
     */
    void addMiddleware(Middleware middleware) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_middlewares.push_back(std::move(middleware));
    }

    /**
     * @brief 清除所有中间件
     */
    void clearMiddlewares() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_middlewares.clear();
    }

    // ==================== 批量操作 ====================

    /**
     * @brief 批量更新
     *
     * 在批量更新期间不通知订阅者，更新完成后一次性通知
     */
    template<typename Func>
    void batchUpdate(Func&& func) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_batchDepth++;

        try {
            func(*this);
        } catch (...) {
            m_batchDepth--;
            throw;
        }

        m_batchDepth--;
        if (m_batchDepth == 0) {
            notifyPendingSubscribers();
        }
    }

private:
    StateStore() = default;

    /**
     * @brief 通知待处理的订阅者
     */
    void notifyPendingSubscribers() {
        // 批量更新完成，通知所有变更的订阅者
        for (const auto& key : m_pendingKeys) {
            auto it = m_subscribers.find(key);
            if (it != m_subscribers.end()) {
                for (const auto& pair : it->second) {
                    pair.second();
                }
            }
        }
        m_pendingKeys.clear();
    }

    std::unordered_map<String, std::any> m_state;
    std::unordered_map<String, std::vector<std::pair<SubscriberId, Subscriber>>> m_subscribers;
    std::unordered_map<SubscriberId, String> m_subscriberToKey;
    std::vector<Middleware> m_middlewares;
    std::vector<String> m_pendingKeys;
    std::atomic<SubscriberId> m_nextSubscriberId{1};
    i32 m_batchDepth = 0;
    mutable std::mutex m_mutex;
};

/**
 * @brief 状态选择器
 *
 * 用于从状态存储中选择和转换数据
 *
 * @tparam T 选择结果类型
 */
template<typename T>
class Selector {
public:
    using SelectFunc = std::function<T(const StateStore&)>;

    /**
     * @brief 构造函数
     * @param select 选择函数
     */
    explicit Selector(SelectFunc select)
        : m_select(std::move(select)) {}

    /**
     * @brief 执行选择
     */
    [[nodiscard]] T select() const {
        return m_select(StateStore::instance());
    }

    /**
     * @brief 订阅选择结果变化
     */
    u64 subscribe(std::function<void(const T&)> subscriber) {
        // 订阅所有依赖键的变化
        return StateStore::instance().subscribe(m_key, [this, subscriber]() {
            T newValue = select();
            subscriber(newValue);
        });
    }

private:
    SelectFunc m_select;
    String m_key; // 用于订阅的键
};

/**
 * @brief 状态绑定点
 *
 * 连接响应式状态和组件
 */
class StateBindingPoint {
public:
    using UpdateCallback = std::function<void()>;

    /**
     * @brief 创建绑定点
     * @param storeKey 状态存储键
     */
    explicit StateBindingPoint(String storeKey)
        : m_storeKey(std::move(storeKey)) {}

    /**
     * @brief 获取值
     */
    template<typename T>
    [[nodiscard]] T get() const {
        return StateStore::instance().get<T>(m_storeKey);
    }

    /**
     * @brief 设置值
     */
    template<typename T>
    void set(const T& value) {
        StateStore::instance().set(m_storeKey, value);
    }

    /**
     * @brief 绑定更新回调
     */
    u64 bind(UpdateCallback callback) {
        return StateStore::instance().subscribe(m_storeKey, std::move(callback));
    }

    /**
     * @brief 解除绑定
     */
    void unbind(u64 id) {
        StateStore::instance().unsubscribe(id);
    }

    /**
     * @brief 获取存储键
     */
    [[nodiscard]] const String& key() const { return m_storeKey; }

private:
    String m_storeKey;
};

} // namespace mc::client::ui::kagero::state
