---

# Kagero UI引擎

**Kagero**（阳炎）是Minecraft Reborn项目的现代化UI引擎，采用模板驱动、响应式状态管理和组件化架构。

命名空间：`mc::client::ui::kagero`

---

## MC Java 1.16.5 UI系统分析

### 核心架构

MC的UI系统由以下核心组件构成：

| 组件 | 职责 |
|------|------|
| **Screen** | 所有UI界面的基类，管理子组件、处理事件、渲染背景/tooltip |
| **ContainerScreen\<T>** | 容器屏幕，与Container配对处理物品槽交互 |
| **Container** | 容器/菜单，服务端客户端共享的物品槽管理逻辑 |
| **Widget** | 所有UI组件基类（Button, TextField, Slider等） |
| **Slot** | 物品槽位，绑定到Container中 |

### 关键特点

1. **纯Java硬编码** - 无模板语言，布局位置直接写死在代码中
2. **无状态管理** - 没有统一的状态管理，数据直接绑定
3. **无事件总线** - 事件直接从Screen传递给子Widget
4. **紧密耦合** - Screen直接持有Container，无MVVM分离

---

## 当前项目UI现状

已有组件：
- `IScreen` 接口 - 基础屏幕接口
- `ScreenManager` - 屏幕栈管理
- `AbstractContainerScreen<T>` - 容器屏幕模板基类
- `InventoryScreen`, `ChatScreen`, `DebugScreen` - 具体屏幕实现
- `GuiRenderer`, `FontRenderer` - 渲染工具

这些老旧组件遵循MC的设计，缺乏现代UI架构的灵活性和可维护性，应该彻底清除并重构为新架构。

---

## 与需求的差距分析

| 需求 | MC现状 | 项目现状 | 目标 |
|------|--------|----------|------|
| **组件化** | 有Widget体系 | 无Widget体系 | 建立完整Widget体系 |
| **模板语言** | 无，硬编码 | 无，硬编码 | 引入模板系统 |
| **状态管理** | 无 | 无 | 实现状态管理系统 |
| **事件总线** | 无，直接传递 | 无 | 实现事件总线 |
| **3D嵌入** | 有drawEntityOnScreen | 预留但未实现 | 实现3D视口组件 |

---

## 详细实施计划

### 第一阶段：UI框架基础架构（预计工作量：核心框架）

#### 1.1 Widget组件体系

```
src/client/ui/kagero/
├── widget/
│   ├── Widget.hpp                  # 组件基类
│   ├── Widget.cpp
│   ├── IWidgetContainer.hpp        # 容器接口
│   ├── ButtonWidget.hpp            # 按钮组件
│   ├── ButtonWidget.cpp
│   ├── ImageButtonWidget.hpp       # 图片按钮
│   ├── TextWidget.hpp              # 文本组件
│   ├── TextWidget.cpp
│   ├── TextFieldWidget.hpp         # 文本输入框
│   ├── TextFieldWidget.cpp
│   ├── SliderWidget.hpp            # 滑块组件
│   ├── SliderWidget.cpp
│   ├── CheckboxWidget.hpp          # 复选框
│   ├── CheckboxWidget.cpp
│   ├── ScrollableWidget.hpp        # 可滚动容器
│   ├── ScrollableWidget.cpp
│   ├── ListWidget.hpp              # 列表组件
│   ├── ListWidget.cpp
│   ├── SlotWidget.hpp              # 物品槽组件
│   ├── SlotWidget.cpp
│   └── Viewport3DWidget.hpp        # 3D视口组件
```

**Widget基类设计**：
```cpp
namespace mc::client::ui::kagero::widget {

class Widget {
public:
    virtual ~Widget() = default;
    
    // 生命周期
    virtual void init() {}
    virtual void tick(f32 dt);
    virtual void render(RenderContext& ctx, i32 mouseX, i32 mouseY, f32 partialTick) = 0;
    virtual void onResize(i32 width, i32 height);
    
    // 事件处理
    virtual bool onClick(i32 mouseX, i32 mouseY, i32 button);
    virtual bool onRelease(i32 mouseX, i32 mouseY, i32 button);
    virtual bool onDrag(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY);
    virtual bool onScroll(i32 mouseX, i32 mouseY, f64 delta);
    virtual bool onKey(i32 key, i32 scanCode, i32 action, i32 mods);
    virtual bool onChar(u32 codePoint);
    
    // 布局
    void setPosition(i32 x, i32 y);
    void setSize(i32 width, i32 height);
    void setAnchor(Anchor anchor);  // 锚点布局
    
    // 状态
    bool isVisible() const;
    bool isActive() const;
    bool isHovered() const;
    bool isFocused() const;
    
    // 层级
    void setZIndex(i32 z);
    void setParent(Widget* parent);
    
protected:
    Rect m_bounds;
    Anchor m_anchor = Anchor::TopLeft;
    bool m_visible = true;
    bool m_active = true;
    bool m_hovered = false;
    bool m_focused = false;
    i32 m_zIndex = 0;
    Widget* m_parent = nullptr;
};

} // namespace mc::client::ui::kagero::widget
```

#### 1.2 事件系统

```
src/client/ui/kagero/
├── event/
│   ├── EventBus.hpp               # 事件总线
│   ├── EventBus.cpp
│   ├── Event.hpp                  # 事件基类
│   ├── InputEvents.hpp            # 输入事件
│   ├── UIEvents.hpp               # UI事件
│   └── WidgetEvents.hpp           # 组件事件
```

**事件系统设计**：
```cpp
namespace mc::client::ui::kagero::event {

// 事件类型
enum class EventType {
    Click,
    Release,
    Drag,
    Scroll,
    KeyPress,
    CharInput,
    Focus,
    Blur,
    ValueChange,
    Custom
};

// 事件基类
class Event {
public:
    virtual ~Event() = default;
    virtual EventType getType() const = 0;
    bool isCancelled() const { return m_cancelled; }
    void cancel() { m_cancelled = true; }
    
private:
    bool m_cancelled = false;
};

// 点击事件
class ClickEvent : public Event {
public:
    EventType getType() const override { return EventType::Click; }
    
    i32 mouseX, mouseY;
    i32 button;
    Widget* target;
};

// 值变化事件
template<typename T>
class ValueChangeEvent : public Event {
public:
    EventType getType() const override { return EventType::ValueChange; }
    
    T oldValue;
    T newValue;
    Widget* source;
};

// 事件总线
class EventBus {
public:
    using HandlerId = u64;
    
    template<typename EventT>
    HandlerId subscribe(std::function<void(const EventT&)> handler);
    
    void unsubscribe(HandlerId id);
    
    template<typename EventT>
    void publish(const EventT& event);
    
    static EventBus& global();
};

} // namespace mc::client::ui::kagero::event
```

#### 1.3 状态管理系统

```
src/client/ui/kagero/
├── state/
│   ├── StateStore.hpp             # 状态存储
│   ├── StateStore.cpp
│   ├── ReactiveState.hpp          # 响应式状态
│   ├── StateBinding.hpp           # 状态绑定
│   └── StateObserver.hpp          # 状态观察者
```

**状态管理设计**：
```cpp
namespace mc::client::ui::kagero::state {

// 响应式状态
template<typename T>
class Reactive {
public:
    using Observer = std::function<void(const T&, const T&)>;
    
    Reactive(T initialValue = T{}) : m_value(std::move(initialValue)) {}
    
    const T& get() const { return m_value; }
    void set(const T& newValue) {
        if (m_value != newValue) {
            T oldValue = m_value;
            m_value = newValue;
            notify(oldValue, newValue);
        }
    }
    
    void observe(Observer observer) {
        m_observers.push_back(std::move(observer));
    }
    
private:
    T m_value;
    std::vector<Observer> m_observers;
    
    void notify(const T& oldVal, const T& newVal) {
        for (auto& obs : m_observers) {
            obs(oldVal, newVal);
        }
    }
};

// 状态存储（类似Redux）
class StateStore {
public:
    using Action = std::function<void(StateStore&)>;
    using Subscriber = std::function<void()>;
    
    // 获取状态
    template<typename T>
    T get(const String& key) const;
    
    // 设置状态
    template<typename T>
    void set(const String& key, T value);
    
    // 派发动作
    void dispatch(Action action);
    
    // 订阅变化
    u64 subscribe(const String& key, Subscriber subscriber);
    void unsubscribe(u64 id);
    
private:
    std::map<String, std::any> m_state;
    std::map<String, std::vector<std::pair<u64, Subscriber>>> m_subscribers;
    u64 m_nextId = 1;
};

} // namespace mc::client::ui::kagero::state
```

### 第二阶段：模板系统（核心创新）（已废弃，以新文件为参考）

#### 2.1 UI模板语言设计

```
src/client/ui/kagero/
├── template/
│   ├── Template.hpp               # 模板解析器
│   ├── Template.cpp
│   ├── TemplateParser.hpp         # 语法解析
│   ├── TemplateParser.cpp
│   ├── TemplateLexer.hpp          # 词法分析
│   ├── TemplateLexer.cpp
│   ├── TemplateAst.hpp            # AST节点
│   ├── TemplateCompiler.hpp       # 模板编译
│   ├── TemplateCompiler.cpp
│   └── bindings/
│       ├── WidgetBinding.hpp      # Widget绑定
│       ├── EventBinding.hpp       # 事件绑定
│       ├── StateBinding.hpp       # 状态绑定
│       └── StyleBinding.hpp       # 样式绑定
```

**模板语法设计**（MC风格）：
```
<!-- inventory_screen.tpl -->
<screen id="inventory" size="176,166" title="container.inventory">
    <!-- 玩家模型视口 -->
    <viewport3d id="playerModel" 
                pos="51,75" 
                size="30,30"
                bind:entity="player"
                bind:rotation="mouseLook"/>
    
    <!-- 物品槽网格 -->
    <grid id="mainInventory"
          pos="8,84"
          cols="9"
          rows="3"
          slotSize="18"
          bind:slots="player.inventory.main">
        <slot bind:item="$slot.item" bind:count="$slot.count"/>
    </grid>
    
    <!-- 快捷栏 -->
    <grid id="hotbar"
          pos="8,142"
          cols="9"
          rows="1"
          slotSize="18"
          bind:slots="player.inventory.hotbar">
        <slot bind:item="$slot.item" bind:count="$slot.count"/>
    </grid>
    
    <!-- 盔甲槽 -->
    <grid id="armor"
          pos="8,8"
          cols="1"
          rows="4"
          slotSize="18"
          bind:slots="player.inventory.armor">
        <slot bind:item="$slot.item" 
              background="item/empty_armor_slot_$index"/>
    </grid>
    
    <!-- 合成区域 -->
    <grid id="crafting"
          pos="98,8"
          cols="2"
          rows="2"
          slotSize="18"
          bind:slots="craftingGrid">
        <slot bind:item="$slot.item"/>
    </grid>
    
    <!-- 合成结果 -->
    <slot id="craftingResult"
          pos="134,28"
          bind:item="craftingResult.item"
          bind:count="craftingResult.count"/>
    
    <!-- 按钮 -->
    <button id="recipeBookBtn"
            pos="104,22"
            size="20,18"
            on:click="toggleRecipeBook()">
        <image src="gui/recipe_button.png"/>
    </button>
    
    <!-- 样式 -->
    <style>
        slot {
            background: "gui/container/inventory.png";
            hover: highlight(0x80FFFFFF);
        }
        
        slot.hover {
            border: 1px solid #FFFFFF;
        }
    </style>
</screen>
```

#### 2.2 模板编译器

将UI模板编译为C++代码：

```cpp
namespace mc::client::ui::kagero::template {

// AST节点类型
enum class NodeType {
    Screen,
    Widget,
    Grid,
    Slot,
    Viewport3D,
    Button,
    Text,
    TextField,
    Image,
    Style,
    Binding
};

// AST节点
struct AstNode {
    NodeType type;
    String tagName;
    std::map<String, String> attributes;
    std::map<String, String> bindings;  // bind:xxx
    std::map<String, String> events;    // on:xxx
    std::vector<std::unique_ptr<AstNode>> children;
    String textContent;
};

// 模板编译器
class TemplateCompiler {
public:
    // 编译模板文件
    std::unique_ptr<CompiledTemplate> compile(const String& source);
    
    // 编译到C++代码（可选，用于性能优化）
    String compileToCpp(const String& templatePath);
};

// 编译后的模板
class CompiledTemplate {
public:
    // 实例化屏幕
    std::unique_ptr<Screen> instantiate(StateStore& state, EventBus& eventBus);
    
    // 更新绑定
    void updateBindings(StateStore& state);
    
private:
    std::unique_ptr<AstNode> m_root;
    std::vector<Binding> m_bindings;
    std::vector<EventHandler> m_eventHandlers;
};

} // namespace mc::client::ui::kagero::template
```




### 第三阶段：布局系统

#### 3.1 布局引擎

```
src/client/ui/kagero/
├── layout/
│   ├── LayoutEngine.hpp           # 布局引擎
│   ├── LayoutEngine.cpp
│   ├── LayoutConstraints.hpp      # 约束布局
│   ├── FlexLayout.hpp             # 弹性布局（类似Flexbox）
│   ├── FlexLayout.cpp
│   ├── GridLayout.hpp             # 网格布局
│   ├── GridLayout.cpp
│   ├── AnchorLayout.hpp           # 锚点布局
│   ├── AnchorLayout.cpp
│   └── LayoutTypes.hpp            # 布局类型定义
```

**布局系统设计**：
```cpp
namespace mc::client::ui::kagero::layout {

// 布局方向
enum class Direction {
    Row,
    Column,
    RowReverse,
    ColumnReverse
};

// 对齐方式
enum class Align {
    Start,
    Center,
    End,
    Stretch
};

// 弹性布局参数
struct FlexItem {
    f32 flexGrow = 0.0f;
    f32 flexShrink = 1.0f;
    f32 flexBasis = 0.0f;
    Align alignSelf = Align::Stretch;
    i32 minWidth = 0;
    i32 minHeight = 0;
    i32 maxWidth = INT_MAX;
    i32 maxHeight = INT_MAX;
    Margin margin;
    Padding padding;
};

// 弹性布局（类似CSS Flexbox）
class FlexLayout {
public:
    void setDirection(Direction dir);
    void setJustifyContent(Align align);
    void setAlignItems(Align align);
    void setGap(i32 gap);
    
    void layout(const Rect& container, std::vector<Widget*>& children);
    
private:
    Direction m_direction = Direction::Row;
    Align m_justify = Align::Start;
    Align m_align = Align::Stretch;
    i32 m_gap = 0;
};

// 网格布局
class GridLayout {
public:
    void setColumns(i32 count);
    void setRows(i32 count);
    void setCellSize(i32 width, i32 height);
    void setGap(i32 gap);
    
    void layout(const Rect& container, std::vector<Widget*>& children);
    
private:
    i32 m_cols = 1;
    i32 m_rows = 1;
    i32 m_cellWidth = 18;
    i32 m_cellHeight = 18;
    i32 m_gap = 0;
};

} // namespace mc::client::ui::kagero::layout
```

### 第四阶段：3D视口组件

#### 4.1 3D嵌入实现

```
src/client/ui/kagero/
├── widget/
│   └── Viewport3DWidget.hpp
│   └── Viewport3DWidget.cpp
├── render/
│   ├── KageroRenderer.hpp         # Kagero渲染器
│   ├── KageroRenderer.cpp
│   ├── ViewportRenderer.hpp       # 视口渲染器
│   └── ViewportRenderer.cpp
```

**3D视口设计**：
```cpp
namespace mc::client::ui::kagero::widget {

class Viewport3DWidget : public Widget {
public:
    Viewport3DWidget();
    ~Viewport3DWidget() override;
    
    // 设置渲染实体
    void setEntity(Entity* entity);
    void setPlayer(Player* player);
    
    // 设置相机参数
    void setCameraDistance(f32 distance);
    void setCameraRotation(f32 pitch, f32 yaw);
    
    // 渲染
    void render(RenderContext& ctx, i32 mouseX, i32 mouseY, f32 partialTick) override;
    
    // 交互
    bool onDrag(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY) override;
    bool onScroll(i32 mouseX, i32 mouseY, f64 delta) override;
    
private:
    void renderEntityToFBO();
    void updateCamera();
    
    Entity* m_entity = nullptr;
    f32 m_cameraDistance = 30.0f;
    f32 m_pitch = 0.0f;
    f32 m_yaw = 0.0f;
    
    // Vulkan资源
    VkFramebuffer m_framebuffer;
    VkRenderPass m_renderPass;
    VkImage m_colorImage;
    VkImageView m_colorView;
    VkDescriptorSet m_descriptorSet;
};

} // namespace mc::client::ui::kagero::widget
```

### 第五阶段：具体UI实现

注意，具体ui不要放kagero下面，我希望kagero是与具体业务逻辑无关的公共UI内核。

#### 5.1 界面实现清单

```
src/client/ui/kagero/
├── screen/
│   ├── main_menu/
│   │   ├── MainMenuScreen.hpp
│   │   ├── MainMenuScreen.cpp
│   │   ├── WorldSelectionScreen.hpp
│   │   ├── WorldSelectionScreen.cpp
│   │   ├── CreateWorldScreen.hpp
│   │   ├── CreateWorldScreen.cpp
│   │   ├── WorldOptionsScreen.hpp
│   │   └── WorldOptionsScreen.cpp
│   ├── inventory/
│   │   ├── PlayerInventoryScreen.hpp    # 玩家背包
│   │   ├── PlayerInventoryScreen.cpp
│   │   ├── ContainerScreen.hpp          # 通用容器屏幕
│   │   ├── ContainerScreen.cpp
│   │   ├── CraftingScreen.hpp           # 工作台
│   │   ├── CraftingScreen.cpp
│   │   ├── FurnaceScreen.hpp            # 熔炉
│   │   ├── FurnaceScreen.cpp
│   │   ├── ChestScreen.hpp              # 箱子
│   │   ├── ChestScreen.cpp
│   │   ├── AnvilScreen.hpp              # 铁砧
│   │   ├── AnvilScreen.cpp
│   │   ├── EnchantmentScreen.hpp        # 附魔台
│   │   ├── EnchantmentScreen.cpp
│   │   ├── BrewingStandScreen.hpp       # 酿造台
│   │   └── BrewingStandScreen.cpp
│   ├── pause/
│   │   ├── PauseScreen.hpp
│   │   ├── PauseScreen.cpp
│   │   ├── OptionsScreen.hpp
│   │   └── OptionsScreen.cpp
│   └── hud/
│       ├── HudRenderer.hpp
│       ├── HudRenderer.cpp
│       ├── HotbarRenderer.hpp
│       ├── HotbarRenderer.cpp
│       ├── HealthBarRenderer.hpp
│       ├── HealthBarRenderer.cpp
│       └── CrosshairRenderer.hpp
```

### 第六阶段：资源系统整合

#### 6.1 UI资源管理

```
src/client/ui/kagero/
├── resource/
│   ├── TextureAtlas.hpp           # UI纹理图集
│   ├── TextureAtlas.cpp
│   ├── Theme.hpp                  # UI主题
│   ├── Theme.cpp
│   ├── FontManager.hpp            # 字体管理
│   └── FontManager.cpp
```

### 完整目录结构

```
src/client/ui/kagero/
├── core/
│   ├── KageroContext.hpp          # Kagero上下文
│   ├── KageroContext.cpp
│   ├── KageroRenderer.hpp         # Kagero渲染器
│   ├── KageroRenderer.cpp
│   ├── InputManager.hpp           # 输入管理
│   └── InputManager.cpp
├── widget/
│   ├── Widget.hpp                 # 组件基类
│   ├── ButtonWidget.hpp/cpp
│   ├── TextWidget.hpp/cpp
│   ├── TextFieldWidget.hpp/cpp
│   ├── SliderWidget.hpp/cpp
│   ├── CheckboxWidget.hpp/cpp
│   ├── ScrollableWidget.hpp/cpp
│   ├── ListWidget.hpp/cpp
│   ├── SlotWidget.hpp/cpp
│   ├── Viewport3DWidget.hpp/cpp
│   └── container/
│       ├── ContainerWidget.hpp
│       ├── StackWidget.hpp
│       └── GridWidget.hpp
├── layout/
│   ├── LayoutEngine.hpp/cpp
│   ├── FlexLayout.hpp/cpp
│   ├── GridLayout.hpp/cpp
│   └── AnchorLayout.hpp/cpp
├── state/
│   ├── StateStore.hpp/cpp
│   ├── ReactiveState.hpp
│   └── StateBinding.hpp
├── event/
│   ├── EventBus.hpp/cpp
│   ├── Event.hpp
│   └── WidgetEvents.hpp
├── template/
│   ├── Template.hpp/cpp
│   ├── TemplateParser.hpp/cpp
│   ├── TemplateLexer.hpp/cpp
│   ├── TemplateAst.hpp
│   ├── TemplateCompiler.hpp/cpp
│   └── bindings/
│       ├── WidgetBinding.hpp
│       ├── EventBinding.hpp
│       └── StateBinding.hpp
├── screen/
│   ├── IScreen.hpp
│   ├── ScreenManager.hpp/cpp
│   ├── AbstractContainerScreen.hpp
│   ├── main_menu/
│   ├── inventory/
│   ├── pause/
│   └── hud/
├── resource/
│   ├── TextureAtlas.hpp/cpp
│   ├── Theme.hpp/cpp
│   └── FontManager.hpp/cpp
├── render/
│   ├── ViewportRenderer.hpp/cpp
│   └── ItemRenderer2D.hpp/cpp
├── Font.hpp/cpp
├── FontRenderer.hpp/cpp
└── FontTextureAtlas.hpp/cpp
```

**命名空间结构**：
- `mc::client::ui::kagero` - 根命名空间
- `mc::client::ui::kagero::widget` - Widget组件
- `mc::client::ui::kagero::event` - 事件系统
- `mc::client::ui::kagero::state` - 状态管理
- `mc::client::ui::kagero::layout` - 布局引擎
- `mc::client::ui::kagero::template` - 模板系统
- `mc::client::ui::kagero::screen` - 屏幕管理
- `mc::client::ui::kagero::resource` - 资源管理
- `mc::client::ui::kagero::render` - 渲染系统

---

## 与MC原始架构的对比

| 方面 | MC Java实现 | Kagero目标 |
|------|-------------|------------|
| **UI定义** | Java硬编码 | 模板语言(.tpl文件) |
| **布局** | 手动计算坐标 | Flexbox/Grid布局引擎 |
| **状态管理** | 直接字段访问 | 响应式状态+事件总线 |
| **组件复用** | 继承Widget | 组合式组件+模板继承 |
| **3D嵌入** | drawEntityOnScreen方法 | Viewport3DWidget组件 |
| **样式** | 代码中硬编码 | 模板内style块 |

---

## 实施优先级

1. **高优先级**：Widget体系、事件系统、布局引擎
2. **中优先级**：状态管理、模板系统
3. **低优先级**：3D视口、主题系统

---

## Kagero命名说明

**Kagero**（陽炎，かげろう）意为"阳炎"或"蜉蝣"，象征着UI系统的特性：

- **轻盈灵动** - 如阳炎般轻量、响应迅速
- **瞬息万变** - 支持动态状态更新和响应式渲染
- **绚丽多彩** - 支持丰富的视觉效果和主题定制

命名空间 `mc::client::ui::kagero` 包含所有Kagero UI引擎的核心组件，确保与项目其他模块清晰分离。
