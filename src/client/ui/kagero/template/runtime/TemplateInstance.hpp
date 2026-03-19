#pragma once

#include "../compiler/TemplateCompiler.hpp"
#include "../binder/BindingContext.hpp"
#include "../../widget/Widget.hpp"
#include "../../widget/IWidgetContainer.hpp"
#include <memory>
#include <unordered_map>
#include <functional>

namespace mc::client::ui::kagero::tpl::runtime {

// 前向声明
class UpdateScheduler;

/**
 * @brief Widget工厂函数类型
 */
using WidgetFactory = std::function<std::unique_ptr<widget::Widget>(
    const String& tagName, const String& id, const std::map<String, String>& attrs)>;

/**
 * @brief 属性设置器函数类型
 */
using AttributeSetter = std::function<void(widget::Widget* widget,
    const String& attrName, const binder::Value& value)>;

/**
 * @brief 事件绑定器函数类型
 */
using EventBinder = std::function<void(widget::Widget* widget,
    const String& eventName, const String& callbackName, binder::BindingContext& ctx)>;

/**
 * @brief 模板实例
 *
 * 运行时的模板实例，包含编译后的模板和运行时状态。
 * 负责将模板实例化为Widget树，并管理绑定和事件。
 *
 * 使用示例：
 * @code
 * // 编译模板
 * TemplateCompiler compiler;
 * auto compiled = compiler.compile(source);
 *
 * // 创建实例
 * BindingContext ctx(store, bus);
 * TemplateInstance instance(compiled.get(), ctx);
 *
 * // 实例化Widget树
 * auto root = instance.instantiate();
 *
 * // 更新绑定
 * instance.updateBindings();
 * @endcode
 */
class TemplateInstance {
public:
    /**
     * @brief 构造函数
     * @param compiled 编译后的模板
     * @param ctx 绑定上下文
     */
    TemplateInstance(const compiler::CompiledTemplate* compiled, binder::BindingContext& ctx);

    /**
     * @brief 析构函数
     */
    ~TemplateInstance();

    // 禁止拷贝
    TemplateInstance(const TemplateInstance&) = delete;
    TemplateInstance& operator=(const TemplateInstance&) = delete;

    // 允许移动
    TemplateInstance(TemplateInstance&&) noexcept;
    TemplateInstance& operator=(TemplateInstance&&) noexcept;

    // ========== Widget工厂注册 ==========

    /**
     * @brief 注册Widget工厂
     *
     * @param tagName 标签名
     * @param factory 工厂函数
     */
    void registerWidgetFactory(const String& tagName, WidgetFactory factory);

    /**
     * @brief 注册默认Widget工厂
     *
     * 为内置Widget类型注册工厂
     */
    void registerDefaultFactories();

    /**
     * @brief 设置默认Widget工厂
     *
     * 当没有找到特定工厂时使用
     */
    void setDefaultFactory(WidgetFactory factory);

    // ========== 属性设置器注册 ==========

    /**
     * @brief 注册属性设置器
     *
     * @param attrName 属性名
     * @param setter 设置函数
     */
    void registerAttributeSetter(const String& attrName, AttributeSetter setter);

    /**
     * @brief 注册默认属性设置器
     */
    void registerDefaultAttributeSetters();

    // ========== 事件绑定器注册 ==========

    /**
     * @brief 注册事件绑定器
     *
     * @param eventName 事件名
     * @param binder 绑定函数
     */
    void registerEventBinder(const String& eventName, EventBinder binder);

    /**
     * @brief 注册默认事件绑定器
     */
    void registerDefaultEventBinders();

    // ========== 实例化 ==========

    /**
     * @brief 实例化Widget树
     *
     * @return 根Widget，失败返回nullptr
     */
    [[nodiscard]] std::unique_ptr<widget::Widget> instantiate();

    /**
     * @brief 实例化到指定容器
     *
     * @param container 目标容器
     * @return 是否成功
     */
    bool instantiateInto(widget::IWidgetContainer* container);

    /**
     * @brief 检查是否已实例化
     */
    [[nodiscard]] bool isInstantiated() const { return m_rootWidget != nullptr; }

    /**
     * @brief 获取根Widget
     */
    [[nodiscard]] widget::Widget* rootWidget() { return m_rootWidget.get(); }
    [[nodiscard]] const widget::Widget* rootWidget() const { return m_rootWidget.get(); }

    // ========== 绑定管理 ==========

    /**
     * @brief 更新所有绑定
     *
     * 从绑定上下文读取最新值并更新Widget
     */
    void updateBindings();

    /**
     * @brief 更新指定路径的绑定
     *
     * @param path 状态路径
     */
    void updateBinding(const String& path);

    /**
     * @brief 设置绑定上下文
     */
    void setBindingContext(binder::BindingContext& ctx) { m_context = &ctx; }

    /**
     * @brief 获取绑定上下文
     */
    [[nodiscard]] binder::BindingContext* bindingContext() { return m_context; }
    [[nodiscard]] const binder::BindingContext* bindingContext() const { return m_context; }

    // ========== 状态更新 ==========

    /**
     * @brief 通知状态变更
     *
     * @param path 变更的状态路径
     */
    void notifyStateChange(const String& path);

    /**
     * @brief 刷新所有绑定
     */
    void refresh();

    // ========== Widget查找 ==========

    /**
     * @brief 通过ID查找Widget
     */
    [[nodiscard]] widget::Widget* findWidgetById(const String& id);

    /**
     * @brief 通过路径查找Widget
     */
    [[nodiscard]] widget::Widget* findWidgetByPath(const String& path);

    // ========== 调试 ==========

    /**
     * @brief 获取实例统计信息
     */
    [[nodiscard]] String debugInfo() const;

private:
    // ========== 实例化辅助方法 ==========

    /**
     * @brief 实例化节点
     */
    [[nodiscard]] std::unique_ptr<widget::Widget> instantiateNode(const ast::Node* node,
        widget::Widget* parent = nullptr);

    /**
     * @brief 实例化元素节点
     */
    [[nodiscard]] std::unique_ptr<widget::Widget> instantiateElement(const ast::ElementNode* element,
        widget::Widget* parent = nullptr);

    /**
     * @brief 实例化文本节点
     */
    [[nodiscard]] std::unique_ptr<widget::Widget> instantiateText(const ast::TextNode* textNode,
        widget::Widget* parent = nullptr);

    /**
     * @brief 创建Widget
     */
    [[nodiscard]] std::unique_ptr<widget::Widget> createWidget(
        const String& tagName, const String& id,
        const std::map<String, String>& attrs);

    /**
     * @brief 应用静态属性
     */
    void applyStaticAttributes(widget::Widget* widget,
        const std::vector<ast::Attribute>& attrs);

    /**
     * @brief 应用绑定属性
     */
    void applyBindingAttributes(widget::Widget* widget,
        const std::vector<ast::Attribute>& attrs,
        const String& widgetPath);

    /**
     * @brief 应用事件绑定
     */
    void applyEventBindings(widget::Widget* widget,
        const std::vector<ast::Attribute>& attrs,
        const String& widgetPath);

    /**
     * @brief 解析静态属性值
     */
    [[nodiscard]] binder::Value parseStaticValue(const ast::Attribute& attr) const;

    /**
     * @brief 从属性创建Widget路径
     */
    [[nodiscard]] String buildWidgetPath(const ast::ElementNode* element,
        const String& parentPath = "") const;

    /**
     * @brief 注册Widget到路径映射
     */
    void registerWidgetPath(const String& path, widget::Widget* widget);

    /**
     * * @brief 注册Widget ID映射
     */
    void registerWidgetId(const String& id, widget::Widget* widget);

private:
    const compiler::CompiledTemplate* m_compiled;
    binder::BindingContext* m_context;

    std::unique_ptr<widget::Widget> m_rootWidget;

    // Widget映射
    std::unordered_map<String, widget::Widget*> m_widgetById;
    std::unordered_map<String, widget::Widget*> m_widgetByPath;

    // 工厂和设置器
    std::unordered_map<String, WidgetFactory> m_widgetFactories;
    std::unordered_map<String, AttributeSetter> m_attributeSetters;
    std::unordered_map<String, EventBinder> m_eventBinders;
    WidgetFactory m_defaultFactory;

    // 订阅ID
    std::vector<u64> m_subscriptionIds;
};

/**
 * @brief 更新调度器
 *
 * 管理模板实例的增量更新，避免频繁刷新整个模板。
 * 支持批量更新、延迟更新和优先级调度。
 */
class UpdateScheduler {
public:
    /**
     * @brief 更新优先级
     */
    enum class Priority : u8 {
        High = 0,       ///< 高优先级（立即更新）
        Normal = 1,     ///< 普通优先级（下一帧更新）
        Low = 2         ///< 低优先级（批量更新）
    };

    /**
     * @brief 更新任务
     */
    struct UpdateTask {
        String path;                ///< 状态路径
        Priority priority;          ///< 优先级
        u64 timestamp;              ///< 创建时间戳
        bool cancelled = false;     ///< 是否取消

        UpdateTask(String p, Priority pri, u64 ts)
            : path(std::move(p)), priority(pri), timestamp(ts) {}
    };

    /**
     * @brief 构造函数
     */
    UpdateScheduler();

    /**
     * @brief 析构函数
     */
    ~UpdateScheduler();

    // 禁止拷贝
    UpdateScheduler(const UpdateScheduler&) = delete;
    UpdateScheduler& operator=(const UpdateScheduler&) = delete;

    // ========== 任务调度 ==========

    /**
     * @brief 调度更新任务
     *
     * @param path 状态路径
     * @param priority 优先级
     * @return 任务ID
     */
    u64 schedule(const String& path, Priority priority = Priority::Normal);

    /**
     * @brief 取消更新任务
     */
    void cancel(u64 taskId);

    /**
     * @brief 取消所有指定路径的任务
     */
    void cancelByPath(const String& path);

    /**
     * @brief 取消所有任务
     */
    void cancelAll();

    // ========== 更新执行 ==========

    /**
     * @brief 执行所有待处理任务
     *
     * @param instance 模板实例
     * @return 执行的任务数量
     */
    u32 executePending(TemplateInstance* instance);

    /**
     * @brief 执行高优先级任务
     */
    u32 executeHighPriority(TemplateInstance* instance);

    /**
     * @brief 执行普通优先级任务
     */
    u32 executeNormalPriority(TemplateInstance* instance);

    /**
     * @brief 执行低优先级任务
     */
    u32 executeLowPriority(TemplateInstance* instance);

    /**
     * @brief 执行批量更新（合并相同路径的更新）
     */
    u32 executeBatch(TemplateInstance* instance);

    // ========== 配置 ==========

    /**
     * @brief 设置批量更新延迟（毫秒）
     */
    void setBatchDelay(u32 delayMs) { m_batchDelayMs = delayMs; }

    /**
     * @brief 设置最大批量大小
     */
    void setMaxBatchSize(u32 size) { m_maxBatchSize = size; }

    /**
     * @brief 设置是否启用延迟更新
     */
    void setDeferredUpdate(bool enabled) { m_deferredUpdate = enabled; }

    // ========== 状态查询 ==========

    /**
     * @brief 获取待处理任务数量
     */
    [[nodiscard]] u32 pendingCount() const;

    /**
     * @brief 检查是否有待处理任务
     */
    [[nodiscard]] bool hasPending() const { return pendingCount() > 0; }

    /**
     * @brief 获取指定优先级的待处理任务数量
     */
    [[nodiscard]] u32 pendingCount(Priority priority) const;

    /**
     * @brief 获取当前时间戳
     */
    [[nodiscard]] u64 currentTimestamp() const;

private:
    /**
     * @brief 执行指定优先级的任务
     */
    u32 executePriority(TemplateInstance* instance, Priority priority);

    /**
     * @brief 去重路径
     */
    void deduplicatePaths();

private:
    std::vector<std::unique_ptr<UpdateTask>> m_tasks;
    std::unordered_map<String, std::vector<u64>> m_pathToTasks;
    u64 m_nextTaskId = 1;
    u64 m_nextTimestamp = 0;

    // 配置
    u32 m_batchDelayMs = 16;    ///< 批量更新延迟（默认16ms）
    u32 m_maxBatchSize = 100;   ///< 最大批量大小
    bool m_deferredUpdate = true; ///< 是否启用延迟更新
};

} // namespace mc::client::ui::kagero::tpl::runtime
