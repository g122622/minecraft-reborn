#include "TemplateInstance.hpp"
#include "../bindings/BuiltinWidgets.hpp"
#include "../bindings/BuiltinEvents.hpp"
#include "../../widget/TextWidget.hpp"
#include "../../event/WidgetEvents.hpp"
#include <algorithm>
#include <chrono>
#include <sstream>

namespace mc::client::ui::kagero::tpl::runtime {

// ========== TemplateInstance实现 ==========

TemplateInstance::TemplateInstance(const compiler::CompiledTemplate* compiled,
                                    binder::BindingContext& ctx)
    : m_compiled(compiled)
    , m_context(&ctx) {
    registerDefaultFactories();
    registerDefaultAttributeSetters();
    registerDefaultEventBinders();
}

TemplateInstance::~TemplateInstance() {
    // 取消所有订阅
    for (u64 id : m_subscriptionIds) {
        if (m_context) {
            m_context->unsubscribe(id);
        }
    }
}

TemplateInstance::TemplateInstance(TemplateInstance&& other) noexcept
    : m_compiled(other.m_compiled)
    , m_context(other.m_context)
    , m_rootWidget(std::move(other.m_rootWidget))
    , m_widgetById(std::move(other.m_widgetById))
    , m_widgetByPath(std::move(other.m_widgetByPath))
    , m_widgetFactories(std::move(other.m_widgetFactories))
    , m_attributeSetters(std::move(other.m_attributeSetters))
    , m_eventBinders(std::move(other.m_eventBinders))
    , m_defaultFactory(std::move(other.m_defaultFactory))
    , m_subscriptionIds(std::move(other.m_subscriptionIds)) {
    other.m_compiled = nullptr;
    other.m_context = nullptr;
}

TemplateInstance& TemplateInstance::operator=(TemplateInstance&& other) noexcept {
    if (this != &other) {
        // 取消现有订阅
        for (u64 id : m_subscriptionIds) {
            if (m_context) {
                m_context->unsubscribe(id);
            }
        }

        m_compiled = other.m_compiled;
        m_context = other.m_context;
        m_rootWidget = std::move(other.m_rootWidget);
        m_widgetById = std::move(other.m_widgetById);
        m_widgetByPath = std::move(other.m_widgetByPath);
        m_widgetFactories = std::move(other.m_widgetFactories);
        m_attributeSetters = std::move(other.m_attributeSetters);
        m_eventBinders = std::move(other.m_eventBinders);
        m_defaultFactory = std::move(other.m_defaultFactory);
        m_subscriptionIds = std::move(other.m_subscriptionIds);

        other.m_compiled = nullptr;
        other.m_context = nullptr;
    }
    return *this;
}

void TemplateInstance::registerWidgetFactory(const String& tagName, WidgetFactory factory) {
    m_widgetFactories[tagName] = std::move(factory);
}

void TemplateInstance::registerDefaultFactories() {
    // 确保BuiltinWidgets已初始化
    bindings::BuiltinWidgets::instance().initialize();

    // 设置默认工厂，使用BuiltinWidgets作为fallback
    m_defaultFactory = [](const String& tagName, const String& id,
                          const std::map<String, String>& attrs) {
        return bindings::BuiltinWidgets::instance().create(tagName, id, attrs);
    };
}

void TemplateInstance::setDefaultFactory(WidgetFactory factory) {
    m_defaultFactory = std::move(factory);
}

void TemplateInstance::registerAttributeSetter(const String& attrName, AttributeSetter setter) {
    m_attributeSetters[attrName] = std::move(setter);
}

void TemplateInstance::registerDefaultAttributeSetters() {
    // 位置属性
    m_attributeSetters["pos"] = [](widget::Widget* widget, const String& attrName,
                                    const binder::Value& value) {
        (void)attrName;
        bindings::widget_attrs::applyPosition(widget, value.toString());
    };

    // 尺寸属性
    m_attributeSetters["size"] = [](widget::Widget* widget, const String& attrName,
                                     const binder::Value& value) {
        (void)attrName;
        bindings::widget_attrs::applySize(widget, value.toString());
    };

    // 可见性
    m_attributeSetters["visible"] = [](widget::Widget* widget, const String& attrName,
                                        const binder::Value& value) {
        (void)attrName;
        widget->setVisible(value.asBool());
    };

    // 激活状态
    m_attributeSetters["active"] = [](widget::Widget* widget, const String& attrName,
                                       const binder::Value& value) {
        (void)attrName;
        widget->setActive(value.asBool());
    };

    // 文本内容
    m_attributeSetters["text"] = [](widget::Widget* widget, const String& attrName,
                                     const binder::Value& value) {
        (void)attrName;
        if (auto* textWidget = dynamic_cast<widget::TextWidget*>(widget)) {
            textWidget->setText(value.toString());
        }
    };

    // 文本颜色
    m_attributeSetters["color"] = [](widget::Widget* widget, const String& attrName,
                                      const binder::Value& value) {
        (void)attrName;
        if (auto* textWidget = dynamic_cast<widget::TextWidget*>(widget)) {
            textWidget->setColor(bindings::widget_attrs::parseColor(value.toString()));
        }
    };

    // X坐标（单独设置）
    m_attributeSetters["x"] = [](widget::Widget* widget, const String& attrName,
                                  const binder::Value& value) {
        (void)attrName;
        i32 x = widget->x();
        i32 y = widget->y();
        widget->setPosition(value.asInteger(), y);
    };

    // Y坐标（单独设置）
    m_attributeSetters["y"] = [](widget::Widget* widget, const String& attrName,
                                  const binder::Value& value) {
        (void)attrName;
        i32 x = widget->x();
        i32 y = widget->y();
        widget->setPosition(x, value.asInteger());
    };

    // 宽度（单独设置）
    m_attributeSetters["width"] = [](widget::Widget* widget, const String& attrName,
                                      const binder::Value& value) {
        (void)attrName;
        i32 w = widget->width();
        i32 h = widget->height();
        widget->setSize(value.asInteger(), h);
    };

    // 高度（单独设置）
    m_attributeSetters["height"] = [](widget::Widget* widget, const String& attrName,
                                       const binder::Value& value) {
        (void)attrName;
        i32 w = widget->width();
        i32 h = widget->height();
        widget->setSize(w, value.asInteger());
    };

    // 锚点
    m_attributeSetters["anchor"] = [](widget::Widget* widget, const String& attrName,
                                       const binder::Value& value) {
        (void)attrName;
        widget->setAnchor(bindings::widget_attrs::parseAnchor(value.toString()));
    };

    // Z层级
    m_attributeSetters["zIndex"] = [](widget::Widget* widget, const String& attrName,
                                       const binder::Value& value) {
        (void)attrName;
        widget->setZIndex(bindings::widget_attrs::parseInt(value.toString()));
    };

    // 透明度
    m_attributeSetters["alpha"] = [](widget::Widget* widget, const String& attrName,
                                      const binder::Value& value) {
        (void)attrName;
        widget->setAlpha(bindings::widget_attrs::parseFloat(value.toString(), 1.0f));
    };
}

void TemplateInstance::registerEventBinder(const String& eventName, EventBinder binder) {
    m_eventBinders[eventName] = std::move(binder);
}

void TemplateInstance::registerDefaultEventBinders() {
    // 确保BuiltinEvents已初始化
    bindings::BuiltinEvents::instance().initialize();

    // 点击事件
    m_eventBinders["click"] = [](widget::Widget* widget, const String& eventName,
                                  const String& callbackName, binder::BindingContext& ctx) {
        (void)eventName;
        if (auto* button = dynamic_cast<widget::ButtonWidget*>(widget)) {
            button->setOnPress([&ctx, callbackName](widget::ButtonWidget& btn) {
                ctx.invokeCallback(callbackName, &btn,
                    event::ButtonClickEvent(&btn));
            });
        }
    };

    // 双击事件
    m_eventBinders["doubleClick"] = [](widget::Widget* widget, const String& eventName,
                                        const String& callbackName, binder::BindingContext& ctx) {
        (void)eventName;
        (void)widget;
        ctx.invokeCallback(callbackName, widget, event::MouseClickEvent(0, 0, 0, 2));
    };

    // 右键点击事件
    m_eventBinders["rightClick"] = [](widget::Widget* widget, const String& eventName,
                                       const String& callbackName, binder::BindingContext& ctx) {
        (void)eventName;
        (void)widget;
        ctx.invokeCallback(callbackName, widget, event::MouseClickEvent(0, 0, 1, 1));
    };

    // 鼠标进入事件
    m_eventBinders["mouseEnter"] = [](widget::Widget* widget, const String& eventName,
                                       const String& callbackName, binder::BindingContext& ctx) {
        (void)eventName;
        (void)callbackName;
        if (widget) {
            widget->setHovered(true);
            widget->onMouseEnter();
        }
    };

    // 鼠标离开事件
    m_eventBinders["mouseLeave"] = [](widget::Widget* widget, const String& eventName,
                                       const String& callbackName, binder::BindingContext& ctx) {
        (void)eventName;
        (void)callbackName;
        if (widget) {
            widget->setHovered(false);
            widget->onMouseLeave();
        }
    };

    // 滚动事件
    m_eventBinders["scroll"] = [](widget::Widget* widget, const String& eventName,
                                   const String& callbackName, binder::BindingContext& ctx) {
        (void)eventName;
        (void)callbackName;
        // ScrollableWidget等需要实现setOnScroll
        (void)widget;
    };

    // 键盘按下事件
    m_eventBinders["keyDown"] = [](widget::Widget* widget, const String& eventName,
                                    const String& callbackName, binder::BindingContext& ctx) {
        (void)eventName;
        (void)callbackName;
        (void)widget;
    };

    // 键盘释放事件
    m_eventBinders["keyUp"] = [](widget::Widget* widget, const String& eventName,
                                  const String& callbackName, binder::BindingContext& ctx) {
        (void)eventName;
        (void)callbackName;
        (void)widget;
    };

    // 焦点获得事件
    m_eventBinders["focus"] = [](widget::Widget* widget, const String& eventName,
                                  const String& callbackName, binder::BindingContext& ctx) {
        (void)eventName;
        (void)callbackName;
        if (widget) {
            widget->setFocused(true);
            widget->onFocusGained();
        }
    };

    // 失去焦点事件
    m_eventBinders["blur"] = [](widget::Widget* widget, const String& eventName,
                                 const String& callbackName, binder::BindingContext& ctx) {
        (void)eventName;
        (void)callbackName;
        if (widget) {
            widget->setFocused(false);
            widget->onFocusLost();
        }
    };

    // 值变化事件
    m_eventBinders["change"] = [](widget::Widget* widget, const String& eventName,
                                   const String& callbackName, binder::BindingContext& ctx) {
        (void)eventName;
        (void)callbackName;
        (void)widget;
    };

    // 输入事件
    m_eventBinders["input"] = [](widget::Widget* widget, const String& eventName,
                                  const String& callbackName, binder::BindingContext& ctx) {
        (void)eventName;
        (void)callbackName;
        (void)widget;
    };
}

std::unique_ptr<widget::Widget> TemplateInstance::instantiate() {
    if (!m_compiled || !m_compiled->isValid()) {
        return nullptr;
    }

    // 清理旧实例
    m_rootWidget.reset();
    m_widgetById.clear();
    m_widgetByPath.clear();

    // 实例化根节点
    const ast::DocumentNode* doc = m_compiled->astRoot();
    if (!doc) return nullptr;

    const ast::ElementNode* rootElement = doc->rootElement();
    if (!rootElement) return nullptr;

    m_rootWidget = instantiateElement(rootElement, nullptr);

    // 设置状态变更订阅
    for (const auto& path : m_compiled->watchedPaths()) {
        u64 subId = m_context->subscribe(path,
            [this](const String& p, const binder::Value&) {
                notifyStateChange(p);
            });
        m_subscriptionIds.push_back(subId);
    }

    return std::move(m_rootWidget);
}

bool TemplateInstance::instantiateInto(widget::IWidgetContainer* container) {
    auto root = instantiate();
    if (!root) return false;

    container->addWidget(std::move(root));
    return true;
}

void TemplateInstance::updateBindings() {
    if (!m_compiled || !m_rootWidget) return;

    for (const auto& plan : m_compiled->bindingPlans()) {
        // 查找Widget
        auto it = m_widgetByPath.find(plan.widgetPath);
        if (it == m_widgetByPath.end()) {
            // 尝试通过ID查找
            it = m_widgetById.find(plan.widgetPath);
            if (it == m_widgetById.end()) continue;
        }

        widget::Widget* widget = it->second;
        if (!widget) continue;

        // 解析绑定值
        binder::Value value = m_context->resolveBinding(
            plan.statePath,
            plan.loopVarName,
            binder::Value()
        );

        // 应用属性
        auto setterIt = m_attributeSetters.find(plan.attributeName);
        if (setterIt != m_attributeSetters.end()) {
            setterIt->second(widget, plan.attributeName, value);
        }
    }
}

void TemplateInstance::updateBinding(const String& path) {
    if (!m_compiled || !m_rootWidget) return;

    for (const auto& plan : m_compiled->bindingPlans()) {
        if (plan.statePath != path) continue;

        auto it = m_widgetByPath.find(plan.widgetPath);
        if (it == m_widgetByPath.end()) {
            it = m_widgetById.find(plan.widgetPath);
            if (it == m_widgetById.end()) continue;
        }

        widget::Widget* widget = it->second;
        if (!widget) continue;

        binder::Value value = m_context->resolveBinding(plan.statePath);

        auto setterIt = m_attributeSetters.find(plan.attributeName);
        if (setterIt != m_attributeSetters.end()) {
            setterIt->second(widget, plan.attributeName, value);
        }
    }
}

void TemplateInstance::notifyStateChange(const String& path) {
    updateBinding(path);
}

void TemplateInstance::refresh() {
    updateBindings();
}

widget::Widget* TemplateInstance::findWidgetById(const String& id) {
    auto it = m_widgetById.find(id);
    return it != m_widgetById.end() ? it->second : nullptr;
}

widget::Widget* TemplateInstance::findWidgetByPath(const String& path) {
    auto it = m_widgetByPath.find(path);
    return it != m_widgetByPath.end() ? it->second : nullptr;
}

String TemplateInstance::debugInfo() const {
    std::ostringstream oss;
    oss << "TemplateInstance:\n";
    oss << "  Valid: " << (m_compiled && m_compiled->isValid() ? "Yes" : "No") << "\n";
    oss << "  Root Widget: " << (m_rootWidget ? "Yes" : "No") << "\n";
    oss << "  Widgets by ID: " << m_widgetById.size() << "\n";
    oss << "  Widgets by Path: " << m_widgetByPath.size() << "\n";
    oss << "  Subscriptions: " << m_subscriptionIds.size() << "\n";
    return oss.str();
}

std::unique_ptr<widget::Widget> TemplateInstance::instantiateNode(const ast::Node* node,
                                                                   widget::Widget* parent) {
    if (!node) return nullptr;

    switch (node->type) {
        case ast::NodeType::TextContent:
            return instantiateText(static_cast<const ast::TextNode*>(node), parent);

        case ast::NodeType::Comment:
            // 跳过注释
            return nullptr;

        default:
            if (auto* element = dynamic_cast<const ast::ElementNode*>(node)) {
                return instantiateElement(element, parent);
            }
            break;
    }

    return nullptr;
}

std::unique_ptr<widget::Widget> TemplateInstance::instantiateElement(
    const ast::ElementNode* element, widget::Widget* parent) {

    if (!element) return nullptr;

    // 1. 条件渲染检查（优先）
    if (element->condition.has_value()) {
        if (!evaluateCondition(element->condition.value())) {
            return nullptr;  // 条件不满足，跳过创建
        }
    }

    // 2. 循环渲染检查
    if (element->loop.has_value()) {
        // 循环元素：为集合中每个项创建子元素
        instantiateLoopChildren(
            element, parent,
            element->loop->collectionPath,
            element->loop->itemVarName,
            element->loop->indexVarName
        );
        return nullptr;  // 循环容器本身不返回Widget
    }

    // 3. 收集属性
    std::map<String, String> staticAttrs;
    for (const auto& attr : element->staticAttrs) {
        staticAttrs[attr.name] = attr.rawValue;
    }

    // 4. 创建Widget
    String id = element->id.empty() ? "" : element->id;
    auto widget = createWidget(element->tagName, id, staticAttrs);
    if (!widget) return nullptr;

    // 5. 设置父Widget容器
    if (auto* containerParent = dynamic_cast<widget::IWidgetContainer*>(parent)) {
        widget->setParent(containerParent);
    }

    // 6. 注册Widget
    String widgetPath = buildWidgetPath(element, parent ? parent->id() : "");
    registerWidgetPath(widgetPath, widget.get());
    if (!id.empty()) {
        registerWidgetId(id, widget.get());
    }

    // 7. 应用静态属性
    applyStaticAttributes(widget.get(), element->staticAttrs);

    // 8. 应用绑定属性（初始值）
    applyBindingAttributes(widget.get(), element->bindingAttrs, widgetPath);

    // 9. 应用事件绑定
    applyEventBindings(widget.get(), element->eventAttrs, widgetPath);

    // 10. 实例化子节点
    for (const auto& child : element->children) {
        auto childWidget = instantiateNode(child.get(), widget.get());
        if (childWidget) {
            // 如果Widget是容器，添加子Widget
            if (auto* container = dynamic_cast<widget::IWidgetContainer*>(widget.get())) {
                container->addWidget(std::move(childWidget));
            }
        }
    }

    return widget;
}

std::unique_ptr<widget::Widget> TemplateInstance::instantiateText(
    const ast::TextNode* textNode, widget::Widget* parent) {

    if (!textNode || textNode->isWhitespace) return nullptr;

    auto widget = std::make_unique<widget::TextWidget>();
    widget->setText(textNode->text);
    if (auto* containerParent = dynamic_cast<widget::IWidgetContainer*>(parent)) {
        widget->setParent(containerParent);
    }

    return widget;
}

std::unique_ptr<widget::Widget> TemplateInstance::createWidget(
    const String& tagName, const String& id,
    const std::map<String, String>& attrs) {

    // 1. 首先使用BuiltinWidgets单例
    auto widget = bindings::BuiltinWidgets::instance().create(tagName, id, attrs);
    if (widget) {
        return widget;
    }

    // 2. 然后使用自定义工厂
    auto it = m_widgetFactories.find(tagName);
    if (it != m_widgetFactories.end()) {
        return it->second(tagName, id, attrs);
    }

    // 3. 最后使用默认工厂
    if (m_defaultFactory) {
        return m_defaultFactory(tagName, id, attrs);
    }

    return nullptr;
}

void TemplateInstance::applyStaticAttributes(widget::Widget* widget,
                                              const std::vector<ast::Attribute>& attrs) {
    if (!widget) return;

    for (const auto& attr : attrs) {
        auto setterIt = m_attributeSetters.find(attr.name);
        if (setterIt != m_attributeSetters.end()) {
            binder::Value value = parseStaticValue(attr);
            setterIt->second(widget, attr.name, value);
        }
    }
}

void TemplateInstance::applyBindingAttributes(widget::Widget* widget,
                                               const std::vector<ast::Attribute>& attrs,
                                               const String& widgetPath) {
    if (!widget || !m_context) return;

    for (const auto& attr : attrs) {
        if (!attr.binding.has_value()) continue;

        // 解析绑定值，支持循环变量
        binder::Value value;

        // 检查绑定路径是否是循环变量引用
        const String& bindingPath = attr.binding->path;
        if (!bindingPath.empty() && bindingPath[0] == '$') {
            // 循环变量引用
            String varName;
            String property;
            size_t dotPos = bindingPath.find('.');
            if (dotPos != String::npos) {
                varName = bindingPath.substr(1, dotPos - 1);
                property = bindingPath.substr(dotPos + 1);
            } else {
                varName = bindingPath.substr(1);
            }

            // 获取循环变量值
            binder::Value loopValue = m_context->getLoopVariable(varName);
            if (!loopValue.isNull()) {
                if (property.empty()) {
                    value = loopValue;
                } else {
                    value = loopValue.getProperty(property);
                }
            }
        } else {
            // 普通绑定路径
            value = m_context->resolveBinding(bindingPath);
        }

        // 应用属性
        auto setterIt = m_attributeSetters.find(attr.baseName());
        if (setterIt != m_attributeSetters.end()) {
            setterIt->second(widget, attr.baseName(), value);
        }
    }
}

void TemplateInstance::applyEventBindings(widget::Widget* widget,
                                           const std::vector<ast::Attribute>& attrs,
                                           const String& widgetPath) {
    if (!widget || !m_context) return;

    for (const auto& attr : attrs) {
        String eventName = attr.baseName();
        String callbackName = attr.callbackName;

        auto binderIt = m_eventBinders.find(eventName);
        if (binderIt != m_eventBinders.end()) {
            binderIt->second(widget, eventName, callbackName, *m_context);
        }
    }
}

binder::Value TemplateInstance::parseStaticValue(const ast::Attribute& attr) const {
    // 根据属性值类型创建Value
    if (std::holds_alternative<String>(attr.value)) {
        return binder::Value(std::get<String>(attr.value));
    } else if (std::holds_alternative<i32>(attr.value)) {
        return binder::Value(std::get<i32>(attr.value));
    } else if (std::holds_alternative<f32>(attr.value)) {
        return binder::Value(std::get<f32>(attr.value));
    } else if (std::holds_alternative<bool>(attr.value)) {
        return binder::Value(std::get<bool>(attr.value));
    }
    return binder::Value();
}

String TemplateInstance::buildWidgetPath(const ast::ElementNode* element,
                                          const String& parentPath) const {
    if (!element) return parentPath;

    if (!element->id.empty()) {
        return parentPath.empty() ? element->id : parentPath + "." + element->id;
    }

    return parentPath.empty() ? element->tagName : parentPath + "." + element->tagName;
}

void TemplateInstance::registerWidgetPath(const String& path, widget::Widget* widget) {
    m_widgetByPath[path] = widget;
}

void TemplateInstance::registerWidgetId(const String& id, widget::Widget* widget) {
    m_widgetById[id] = widget;
}

void TemplateInstance::instantiateLoopChildren(
    const ast::ElementNode* element,
    widget::Widget* parent,
    const String& collectionPath,
    const String& itemVarName,
    const String& indexVarName) {

    if (!element || !m_context) return;

    // 解析集合
    auto collection = resolveCollection(collectionPath);

    // 获取父容器
    auto* container = dynamic_cast<widget::IWidgetContainer*>(parent);
    if (!container) return;

    // 为每个元素创建子节点
    for (size_t i = 0; i < collection.size(); ++i) {
        // 设置循环变量
        m_context->setLoopVariable(itemVarName, collection[i]);
        if (!indexVarName.empty()) {
            m_context->setLoopVariable(indexVarName, binder::Value(static_cast<i32>(i)));
        }

        // 实例化子节点
        for (const auto& child : element->children) {
            auto childWidget = instantiateNode(child.get(), parent);
            if (childWidget) {
                container->addWidget(std::move(childWidget));
            }
        }

        // 清除循环变量
        m_context->clearLoopVariable(itemVarName);
        if (!indexVarName.empty()) {
            m_context->clearLoopVariable(indexVarName);
        }
    }
}

std::vector<binder::Value> TemplateInstance::resolveCollection(const String& path) const {
    if (!m_context) return {};

    return m_context->resolveCollection(path);
}

bool TemplateInstance::evaluateCondition(const ast::ConditionInfo& condition) const {
    if (!m_context) return false;

    auto value = m_context->resolveBinding(condition.booleanPath);
    bool visible = value.asBool();

    if (condition.negate) {
        visible = !visible;
    }

    return visible;
}

// ========== UpdateScheduler实现 ==========

UpdateScheduler::UpdateScheduler()
    : m_batchDelayMs(16)
    , m_maxBatchSize(100)
    , m_deferredUpdate(true) {
}

UpdateScheduler::~UpdateScheduler() {
    cancelAll();
}

u64 UpdateScheduler::schedule(const String& path, Priority priority) {
    auto task = std::make_unique<UpdateTask>(path, priority, m_nextTimestamp++);
    u64 taskId = reinterpret_cast<u64>(task.get());
    m_pathToTasks[path].push_back(taskId);
    m_tasks.push_back(std::move(task));
    return taskId;
}

void UpdateScheduler::cancel(u64 taskId) {
    for (auto& task : m_tasks) {
        if (reinterpret_cast<u64>(task.get()) == taskId) {
            task->cancelled = true;
            // 从路径映射中移除
            auto it = m_pathToTasks.find(task->path);
            if (it != m_pathToTasks.end()) {
                auto& ids = it->second;
                ids.erase(std::remove(ids.begin(), ids.end(), taskId), ids.end());
            }
            break;
        }
    }
}

void UpdateScheduler::cancelByPath(const String& path) {
    auto it = m_pathToTasks.find(path);
    if (it != m_pathToTasks.end()) {
        for (u64 taskId : it->second) {
            for (auto& task : m_tasks) {
                if (reinterpret_cast<u64>(task.get()) == taskId) {
                    task->cancelled = true;
                    break;
                }
            }
        }
        m_pathToTasks.erase(it);
    }
}

void UpdateScheduler::cancelAll() {
    m_tasks.clear();
    m_pathToTasks.clear();
}

u32 UpdateScheduler::executePending(TemplateInstance* instance) {
    if (!instance) return 0;

    deduplicatePaths();

    u32 count = 0;
    count += executeHighPriority(instance);
    count += executeNormalPriority(instance);
    count += executeLowPriority(instance);

    // 清理已完成的任务
    m_tasks.erase(
        std::remove_if(m_tasks.begin(), m_tasks.end(),
            [](const std::unique_ptr<UpdateTask>& task) {
                return task->cancelled;
            }),
        m_tasks.end()
    );

    return count;
}

u32 UpdateScheduler::executeHighPriority(TemplateInstance* instance) {
    return executePriority(instance, Priority::High);
}

u32 UpdateScheduler::executeNormalPriority(TemplateInstance* instance) {
    return executePriority(instance, Priority::Normal);
}

u32 UpdateScheduler::executeLowPriority(TemplateInstance* instance) {
    return executePriority(instance, Priority::Low);
}

u32 UpdateScheduler::executeBatch(TemplateInstance* instance) {
    if (!instance) return 0;

    deduplicatePaths();

    u32 count = 0;
    std::set<String> processedPaths;

    for (const auto& task : m_tasks) {
        if (task->cancelled) continue;
        if (processedPaths.count(task->path) > 0) continue;

        instance->updateBinding(task->path);
        processedPaths.insert(task->path);
        task->cancelled = true;
        ++count;

        if (processedPaths.size() >= m_maxBatchSize) break;
    }

    return count;
}

u32 UpdateScheduler::pendingCount() const {
    u32 count = 0;
    for (const auto& task : m_tasks) {
        if (!task->cancelled) ++count;
    }
    return count;
}

u32 UpdateScheduler::pendingCount(Priority priority) const {
    u32 count = 0;
    for (const auto& task : m_tasks) {
        if (!task->cancelled && task->priority == priority) ++count;
    }
    return count;
}

u64 UpdateScheduler::currentTimestamp() const {
    return m_nextTimestamp;
}

u32 UpdateScheduler::executePriority(TemplateInstance* instance, Priority priority) {
    if (!instance) return 0;

    u32 count = 0;
    for (auto& task : m_tasks) {
        if (task->cancelled) continue;
        if (task->priority != priority) continue;

        instance->updateBinding(task->path);
        task->cancelled = true;
        ++count;
    }

    return count;
}

void UpdateScheduler::deduplicatePaths() {
    // 对每个路径只保留最新任务
    for (auto& [path, taskIds] : m_pathToTasks) {
        if (taskIds.size() <= 1) continue;

        // 找到最新任务（最高时间戳）
        u64 latestTaskId = 0;
        u64 latestTimestamp = 0;
        for (u64 taskId : taskIds) {
            for (const auto& task : m_tasks) {
                if (reinterpret_cast<u64>(task.get()) == taskId &&
                    task->timestamp > latestTimestamp) {
                    latestTaskId = taskId;
                    latestTimestamp = task->timestamp;
                }
            }
        }

        // 取消其他任务
        for (u64 taskId : taskIds) {
            if (taskId != latestTaskId) {
                for (auto& task : m_tasks) {
                    if (reinterpret_cast<u64>(task.get()) == taskId) {
                        task->cancelled = true;
                        break;
                    }
                }
            }
        }
    }
}

} // namespace mc::client::ui::kagero::tpl::runtime
