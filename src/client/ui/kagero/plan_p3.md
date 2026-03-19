# 🔄 第三阶段：Kagero 布局系统（完善版）

> 核心原则：**声明式布局 + 响应式重排 + 零手动坐标计算**

---

## 🎯 用户如何使用布局系统？

### 方式一：模板中声明（推荐，90%场景）

```xml
<!-- 使用弹性布局 -->
<flex direction="row" justify="center" align="center" gap="8">
    <button id="btn1" size="100,40">按钮1</button>
    <button id="btn2" size="100,40" flex:grow="1">自适应按钮</button>
    <button id="btn3" size="100,40">按钮3</button>
</flex>

<!-- 使用网格布局（适合物品栏） -->
<grid id="inventoryGrid" 
      cols="9" rows="3" 
      cellSize="18,18" 
      gap="2"
      bind:items="player.inventory">
    <slot bind:item="$item.id"/>
</grid>

<!-- 使用锚点布局（适合绝对定位+相对锚点） -->
<anchor>
    <button id="closeBtn" 
            anchor:top="10" 
            anchor:right="10"
            size="30,30">✕</button>
    
    <text id="title"
          anchor:top="10"
          anchor:left="50%"
          anchor:offsetX="-50%">游戏标题</text>
</anchor>

<!-- 混合布局：外层flex + 内层grid -->
<flex direction="column" gap="16" padding="16">
    <text bind:text="screen.title" style="title"/>
    
    <grid cols="4" rows="2" cellSize="64,64" gap="8">
        <icon bind:src="$icon.path"/>
    </grid>
    
    <flex direction="row" justify="end" gap="8">
        <button on:click="onCancel">取消</button>
        <button on:click="onConfirm" style="primary">确认</button>
    </flex>
</flex>
```

### 方式二：C++ 代码中配置（动态布局场景）

```cpp
// 创建容器并设置布局
auto container = std::make_unique<ContainerWidget>();

// 弹性布局配置
auto* flex = container->setLayout<layout::FlexLayout>();
flex->setDirection(layout::Direction::Row);
flex->setJustifyContent(layout::Align::Center);
flex->setAlignItems(layout::Align::Center);
flex->setGap(8);

// 添加子项并配置弹性参数
auto* btn1 = container->addChild<widget::ButtonWidget>("按钮1");
btn1->setSize(100, 40);

auto* btn2 = container->addChild<widget::ButtonWidget>("自适应");
btn2->setSize(100, 40);
btn2->getFlexItem().flexGrow = 1.0f;  // 占据剩余空间
btn2->getFlexItem().minWidth = 50;    // 最小宽度限制

// 触发初始布局
container->requestLayout();
```

### 方式三：混合模式（模板声明 + C++ 动态调整）

```xml
<!-- 模板声明基础布局 -->
<flex id="mainLayout" direction="column" bind:gap="ui.spacing">
    <!-- 内容区域 -->
    <container id="content" flex:grow="1"/>
    <!-- 底部工具栏 -->
    <flex id="toolbar" direction="row" justify="space-between">
        <text bind:text="player.coords"/>
        <button on:click="toggleSettings">⚙️</button>
    </flex>
</flex>
```

```cpp
// C++ 动态调整布局参数（响应游戏状态）
void GameUI::onSettingsChanged(const Settings& s) {
    auto* layout = screen->queryById<ContainerWidget>("mainLayout")->getFlexLayout();
    
    // 绑定状态：当 ui.spacing 变化时，layout 自动重排
    stateStore.observe("ui.spacing", [layout](const auto& oldVal, const auto& newVal) {
        layout->setGap(newVal.asInt());
        layout->markDirty();  // 标记需要重布局
    });
}
```

---

## 🔗 与其他系统的整合设计

### 整合关系图

```
┌─────────────────────────────────────┐
│           Template Parser           │
│  解析 <flex gap="{{ui.spacing}}">   │
└────────┬────────────────────────────┘
         │ 生成布局配置 + 绑定表达式
         ▼
┌─────────────────────────────────────┐
│          BindingContext             │
│  • 解析 gap="{{ui.spacing}}"        │
│  • 连接 StateStore 观察变化          │
└────────┬────────────────────────────┘
         │ 状态变化时通知
         ▼
┌─────────────────────────────────────┐
│          LayoutEngine               │
│  • 接收 Widget 树 + 布局配置         │
│  • 计算每个 Widget 的最终 Rect       │
│  • 支持增量重排（脏标记优化）        │
└────────┬────────────────────────────┘
         │ 输出计算后的位置/大小
         ▼
┌─────────────────────────────────────┐
│            Widget                   │
│  • 应用 layoutResult 到 m_bounds    │
│  • 触发 onResize() 回调             │
│  • 标记需要重渲染                    │
└────────┬────────────────────────────┘
         │ 渲染帧调用
         ▼
┌─────────────────────────────────────┐
│         RenderContext               │
│  • 使用 Widget::getBounds() 渲染    │
└─────────────────────────────────────┘
```

---

## 🏗️ 完善后的目录结构

```
src/client/ui/kagero/layout/
├── core/
│   ├── LayoutEngine.hpp            # 布局引擎主类
│   ├── LayoutEngine.cpp
│   ├── LayoutResult.hpp            # 布局计算结果
│   ├── LayoutResult.cpp
│   ├── LayoutContext.hpp           # 布局计算上下文（传递约束等）
│   └── LayoutContext.cpp
├── constraints/
│   ├── LayoutConstraints.hpp       # 约束条件定义
│   ├── LayoutConstraints.cpp
│   ├── MeasureSpec.hpp             # 测量规格（类似Android）
│   ├── MeasureSpec.cpp
│   └── SizeConstraint.hpp          # 尺寸约束工具
├── algorithms/
│   ├── FlexLayout.hpp              # 弹性布局算法
│   ├── FlexLayout.cpp
│   ├── GridLayout.hpp              # 网格布局算法
│   ├── GridLayout.cpp
│   ├── AnchorLayout.hpp            # 锚点布局算法
│   ├── AnchorLayout.cpp
│   ├── StackLayout.hpp             # 堆叠布局（绝对定位）
│   ├── StackLayout.cpp
│   └── LayoutUtils.hpp             # 通用布局工具函数
├── integration/
│   ├── WidgetLayoutAdaptor.hpp     # Widget ↔ Layout 适配器
│   ├── WidgetLayoutAdaptor.cpp
│   ├── TemplateLayoutBinder.hpp    # 模板属性 → 布局参数
│   ├── TemplateLayoutBinder.cpp
│   ├── StateLayoutObserver.hpp     # 状态变化 → 布局更新
│   └── StateLayoutObserver.cpp
├── responsive/
│   ├── Breakpoint.hpp              # 响应式断点定义
│   ├── MediaQuery.hpp              # 媒体查询（类似CSS）
│   ├── MediaQuery.cpp
│   └── ResponsiveLayout.hpp        # 响应式布局容器
└── debug/
    ├── LayoutVisualizer.hpp        # 布局调试可视化
    ├── LayoutVisualizer.cpp
    └── LayoutProfiler.hpp          # 布局性能分析
```

---

## 🔧 核心组件详细设计

### 1. 布局约束与测量规格（类似Android）

```cpp
namespace mc::client::ui::kagero::layout {

// 测量模式：父容器给子元素的尺寸约束
enum class MeasureMode {
    Unspecified,  // 无约束，子元素想要多大就多大
    Exactly,      // 精确值，子元素必须用这个尺寸
    AtMost        // 最大值，子元素不能超过这个尺寸
};

// 测量规格：尺寸 + 模式
struct MeasureSpec {
    i32 size;
    MeasureMode mode;
    
    static MeasureSpec MakeExactly(i32 size) { return {size, MeasureMode::Exactly}; }
    static MeasureSpec MakeAtMost(i32 size) { return {size, MeasureMode::AtMost}; }
    static MeasureSpec MakeUnspecified() { return {0, MeasureMode::Unspecified}; }
};

// 布局约束：子元素向父元素声明的需求
struct LayoutConstraints {
    // 期望尺寸
    i32 minWidth = 0;
    i32 minHeight = 0;
    i32 preferredWidth = -1;  // -1 表示无偏好
    i32 preferredHeight = -1;
    i32 maxWidth = INT_MAX;
    i32 maxHeight = INT_MAX;
    
    // 边距（影响父容器分配空间）
    Margin margin;
    
    // 弹性参数（Flex布局专用）
    struct FlexParams {
        f32 grow = 0.0f;
        f32 shrink = 1.0f;
        f32 basis = 0.0f;
        Align selfAlign = Align::Stretch;
    } flex;
    
    // 是否参与布局（用于条件渲染）
    bool enabled = true;
    
    // 工具函数：根据父容器规格计算实际可用空间
    MeasureSpec resolveWidth(const MeasureSpec& parentSpec) const;
    MeasureSpec resolveHeight(const MeasureSpec& parentSpec) const;
};

} // namespace layout
```

### 2. 布局结果与Widget适配器

```cpp
namespace mc::client::ui::kagero::layout {

// 布局计算结果
struct LayoutResult {
    Rect bounds;              // 最终位置+大小（相对于父容器）
    bool needsRepaint = true; // 是否需要重渲染
    bool needsRelayout = false; // 是否需要重新布局（子元素变化时）
    
    // 调试信息
    #ifdef KAGERO_LAYOUT_DEBUG
    String algorithmUsed;
    f64 computeTimeMs;
    #endif
};

// Widget ↔ Layout 适配器：让任意Widget支持布局
class WidgetLayoutAdaptor {
public:
    WidgetLayoutAdaptor(widget::Widget* widget);
    
    // 🔹 获取子元素对该布局器的约束需求
    LayoutConstraints getConstraints() const;
    
    // 🔹 测量子元素在给定规格下的期望尺寸
    Size measure(const MeasureSpec& widthSpec, const MeasureSpec& heightSpec) const;
    
    // 🔹 应用布局结果到Widget
    void applyLayout(const LayoutResult& result);
    
    // 🔹 标记需要重布局（当内容变化时调用）
    void requestLayout();
    
    // 🔹 标记需要重渲染（当样式/内容变化但尺寸不变时）
    void requestRender();
    
    // 🔹 获取关联的Widget
    widget::Widget* getWidget() const { return m_widget; }
    
private:
    widget::Widget* m_widget;
    bool m_layoutDirty = true;
    bool m_renderDirty = true;
    
    // 缓存上次测量结果（避免重复计算）
    mutable std::optional<Size> m_lastMeasuredSize;
    mutable MeasureSpec m_lastWidthSpec, m_lastHeightSpec;
};

} // namespace layout
```

### 3. 布局引擎主类（支持增量重排）

```cpp
namespace mc::client::ui::kagero::layout {

class LayoutEngine {
public:
    LayoutEngine();
    
    // 🔹 执行完整布局计算
    void layout(WidgetLayoutAdaptor* root, const Rect& availableSpace);
    
    // 🔹 执行增量布局（只重排dirty的子树）
    void layoutDirty(WidgetLayoutAdaptor* root);
    
    // 🔹 注册布局算法（支持扩展）
    void registerAlgorithm(String name, std::unique_ptr<LayoutAlgorithm> algo);
    
    // 🔹 获取性能统计（调试用）
    struct LayoutStats {
        i32 totalWidgets;
        i32 relayoutedWidgets;
        f64 totalTimeMs;
        f64 maxDepth;
    };
    LayoutStats getLastStats() const { return m_stats; }
    
    // 🔹 启用/禁用调试可视化
    void setDebugVisualize(bool enabled);
    
    static LayoutEngine& global();
    
private:
    // 递归布局实现
    LayoutResult layoutNode(WidgetLayoutAdaptor* node, 
                           const MeasureSpec& widthSpec,
                           const MeasureSpec& heightSpec,
                           i32 depth);
    
    // 根据Widget的layout-type选择算法
    LayoutAlgorithm* selectAlgorithm(const WidgetLayoutAdaptor* node);
    
    // 性能统计
    LayoutStats m_stats;
    bool m_debugVisualize = false;
    
    // 注册的算法表
    std::map<String, std::unique_ptr<LayoutAlgorithm>> m_algorithms;
};

// 布局算法基类（策略模式）
class LayoutAlgorithm {
public:
    virtual ~LayoutAlgorithm() = default;
    
    // 核心：根据子元素约束计算布局
    virtual LayoutResult compute(const Rect& container,
                                const std::vector<WidgetLayoutAdaptor*>& children,
                                const LayoutConstraints& parentConstraints) = 0;
    
    // 获取算法名称（用于调试/模板解析）
    virtual String getName() const = 0;
};

} // namespace layout
```

### 4. 模板 ↔ 布局 绑定器

```cpp
namespace mc::client::ui::kagero::template::binder {

// 解析模板中的布局属性并绑定到Widget
class TemplateLayoutBinder {
public:
    // 解析 <flex direction="row" gap="{{ui.spacing}}">
    static bool bindLayoutAttributes(widget::Widget* widget,
                                    const ast::WidgetNode* astNode,
                                    BindingContext& ctx);
    
    // 解析子元素的弹性参数 <button flex:grow="1" flex:minWidth="50">
    static bool bindFlexParameters(widget::Widget* widget,
                                const ast::WidgetNode* astNode,
                                BindingContext& ctx);
    
private:
    // 辅助：解析尺寸值（支持 "100", "50%", "calc(100%-20)"）
    static std::optional<Dimension> parseDimension(const String& value);
    
    // 辅助：解析对齐枚举
    static std::optional<Align> parseAlign(const String& value);
};

// 响应式布局：根据屏幕尺寸自动切换布局策略
class ResponsiveLayoutBinder {
public:
    // 解析 <flex responsive:mobile="column" responsive:tablet="row">
    static void bindResponsiveAttributes(widget::Widget* widget,
                                        const ast::WidgetNode* astNode,
                                        BindingContext& ctx);
    
    // 注册媒体查询回调
    static void observeMediaQuery(const String& query,
                                 std::function<void(bool matches)> callback);
};

} // namespace binder
```

---

## 🎨 模板语法扩展（布局相关属性）

### 容器布局属性

```xml
<!-- 弹性布局容器 -->
<flex 
    direction="row|column|row-reverse|column-reverse"
    justify="start|center|end|space-between|space-around"
    align="start|center|end|stretch"
    wrap="nowrap|wrap|wrap-reverse"
    gap="8"
    padding="16"
    bind:gap="ui.spacing"
>
    <!-- 子元素 -->
</flex>

<!-- 网格布局容器 -->
<grid
    cols="3"
    rows="auto"
    colWidth="64"
    rowHeight="64"
    gap="4"
    autoFlow="row|column|dense"
    bind:cols="inventory.columns"
>
    <!-- 子元素：可指定跨越行列 -->
    <slot grid:colSpan="2" grid:rowSpan="1"/>
</grid>

<!-- 锚点布局容器 -->
<anchor>
    <!-- 子元素：相对于容器边缘定位 -->
    <button 
        anchor:left="10" 
        anchor:top="10"
        anchor:right="auto"   <!-- auto表示不约束 -->
        anchor:bottom="auto"
        anchor:offsetX="0"
        anchor:offsetY="0"
    />
</anchor>
```

### 子元素布局参数

```xml
<flex>
    <!-- 弹性参数 -->
    <button 
        flex:grow="1"           <!-- 占据剩余空间的比例 -->
        flex:shrink="0"         <!-- 空间不足时是否缩小 -->
        flex:basis="100"        <!-- 初始基准尺寸 -->
        flex:alignSelf="center" <!-- 覆盖容器的align-items -->
        flex:minWidth="50"      <!-- 最小宽度 -->
        flex:maxWidth="200"     <!-- 最大宽度 -->
    />
    
    <!-- 网格参数 -->
    <slot 
        grid:col="2"            <!-- 起始列（1-based） -->
        grid:row="1"            <!-- 起始行 -->
        grid:colSpan="2"        <!-- 跨越列数 -->
        grid:rowSpan="1"        <!-- 跨越行数 -->
    />
    
    <!-- 通用布局参数 -->
    <text
        margin="8,16,8,16"      <!-- 上右下左 -->
        padding="4,8"           <!-- 上下,左右 -->
        alignSelf="center"
    />
</flex>
```

### 响应式布局（媒体查询）

```xml
<!-- 根据屏幕宽度切换布局方向 -->
<flex 
    direction="row"
    responsive:direction@maxWidth:768="column"
    responsive:direction@maxWidth:480="column-reverse"
>
    <button>Btn1</button>
    <button>Btn2</button>
</flex>

<!-- 根据横竖屏切换网格列数 -->
<grid
    cols="4"
    responsive:cols@orientation:landscape="6"
    responsive:cols@orientation:portrait="3"
>
    <icon/>
</grid>

<!-- 条件显示（类似CSS媒体查询） -->
<tooltip 
    visible="true"
    responsive:visible@maxWidth:480="false"  <!-- 小屏隐藏 -->
>
    <text>高级提示</text>
</tooltip>
```

---

## 🔄 响应式更新流程

### 状态变化 → 布局更新 → 渲染更新

```cpp
// 1. C++ 修改状态
stateStore.set("ui.spacing", 16);

// 2. StateLayoutObserver 检测到变化
class StateLayoutObserver {
    void observe(const String& path, WidgetLayoutAdaptor* widget) {
        stateStore.subscribe(path, [widget, path](const auto& oldVal, const auto& newVal) {
            // 解析路径：如果绑定的是布局参数
            if (path.ends_with(".spacing") || path.ends_with(".gap")) {
                widget->getWidget()->requestLayout();  // 标记重布局
            } else {
                widget->getWidget()->requestRender();  // 仅重渲染
            }
        });
    }
};

// 3. Widget 标记 dirty
void Widget::requestLayout() {
    m_layoutAdaptor->markDirty();
    if (m_parent) {
        m_parent->requestLayout();  // 向上传播（父容器可能需要重排）
    }
}

// 4. 下一帧渲染前执行布局
void ScreenManager::preRender(f32 partialTick) {
    if (m_rootWidget->isLayoutDirty()) {
        LayoutEngine::global().layoutDirty(m_rootWidget->getLayoutAdaptor());
    }
    // 然后执行渲染...
}
```

### 增量布局优化（避免全树重排）

```cpp
// LayoutEngine::layoutDirty 实现
void LayoutEngine::layoutDirty(WidgetLayoutAdaptor* root) {
    // 只遍历 dirty 的子树
    std::vector<WidgetLayoutAdaptor*> dirtyNodes;
    collectDirtyNodes(root, dirtyNodes);
    
    // 按深度排序，确保父节点先于子节点布局
    std::sort(dirtyNodes.begin(), dirtyNodes.end(), 
              [](auto* a, auto* b) { return a->getDepth() < b->getDepth(); });
    
    // 逐个重布局
    for (auto* node : dirtyNodes) {
        if (node->isDirty()) {
            auto parentSpec = computeParentMeasureSpec(node);
            auto result = layoutNode(node, parentSpec.width, parentSpec.height, 0);
            node->applyLayout(result);
            node->clearDirty();
        }
    }
}

// 收集dirty节点（含子树优化）
void collectDirtyNodes(WidgetLayoutAdaptor* node, std::vector<WidgetLayoutAdaptor*>& out) {
    if (node->isLayoutDirty()) {
        out.push_back(node);
        // 优化：如果父节点已经dirty，子节点不需要单独加入（会被父节点重排时处理）
        if (!node->getParent() || !node->getParent()->isLayoutDirty()) {
            for (auto* child : node->getChildren()) {
                collectDirtyNodes(child, out);
            }
        }
    }
}
```

---

## 🛠️ 调试与开发工具

### 布局可视化调试

```cpp
// 启用调试模式（开发环境）
#ifdef KAGERO_DEBUG
LayoutEngine::global().setDebugVisualize(true);
#endif

// 渲染时绘制布局辅助线
void LayoutVisualizer::render(RenderContext& ctx, WidgetLayoutAdaptor* root) {
    for (auto* node : collectAllNodes(root)) {
        auto bounds = node->getWidget()->getBounds();
        
        // 绘制边框
        ctx.drawRect(bounds, Color(0xFF00FF80), 1.0f);  // 半透明绿色
        
        // 绘制margin（黄色虚线）
        if (node->getConstraints().margin.any()) {
            ctx.drawRect(bounds.expand(node->getConstraints().margin), 
                        Color(0xFFFFA500), 1.0f, true);  // dashed
        }
        
        // 绘制padding（蓝色区域）
        auto contentBounds = bounds.shrink(node->getConstraints().padding);
        ctx.fillRect(contentBounds, Color(0x200080FF));
    }
}
```

### 布局性能分析

```cpp
// 性能统计输出（每帧或手动触发）
void LayoutProfiler::printStats() {
    auto stats = LayoutEngine::global().getLastStats();
    Logger::info("Layout Stats: {} widgets, {} relayouted, {:.2f}ms, depth={}",
                stats.totalWidgets, stats.relayoutedWidgets, 
                stats.totalTimeMs, stats.maxDepth);
    
    // 警告：如果布局耗时过长
    if (stats.totalTimeMs > 5.0) {
        Logger::warn("Layout performance warning: {}ms > 5ms threshold", stats.totalTimeMs);
    }
}
```

### 优先级建议

| 优先级 | 模块 | 理由 |
|--------|------|------|
| 🔴 高 | `LayoutConstraints` + `WidgetLayoutAdaptor` | 所有布局的基础抽象 |
| 🔴 高 | `FlexLayout` 算法 | 最常用，覆盖80%场景 |
| 🟡 中 | 模板属性绑定 | 让设计师/策划能直接使用 |
| 🟡 中 | 增量布局优化 | 性能关键，但可后续优化 |
| 🟢 低 | 响应式媒体查询 | 高级特性，可二期实现 |
| 🟢 低 | 布局可视化调试 | 开发体验提升，非功能必需 |

---

## 🎁 快速开始示例

### 1. 创建带布局的屏幕（模板方式）

```xml
<!-- screens/main_menu.tpl -->
<screen id="mainMenu" size="100%,100%">
    <flex direction="column" justify="center" align="center" gap="16" padding="32">
        <text bind:text="game.title" style="title"/>
        
        <flex direction="column" gap="8" flex:grow="1" justify="center">
            <button id="btnStart" size="200,50" on:click="startGame">开始游戏</button>
            <button id="btnOptions" size="200,50" on:click="openOptions">选项</button>
            <button id="btnExit" size="200,50" on:click="exitGame">退出</button>
        </flex>
        
        <text bind:text="game.version" style="footer"/>
    </flex>
</screen>
```

### 2. C++ 侧注册逻辑

```cpp
// main_menu_screen.cpp
class MainMenuScreen : public Screen {
public:
    void onInit() override {
        // 加载并实例化模板
        auto compiled = TemplateCompiler::global().compileFile("screens/main_menu.tpl");
        m_root = compiled->instantiate(m_bindingCtx);
        
        // 注册事件回调
        m_bindingCtx.exposeCallback("startGame", [this](auto*, auto&) {
            Game::get().startNewWorld();
        });
        
        // 初始布局
        m_root->requestLayout();
    }
    
    void onResize(i32 w, i32 h) override {
        // 窗口大小变化时自动重布局
        m_root->setSize(w, h);
        m_root->requestLayout();  // FlexLayout 会自动重新计算子元素位置
    }
};
```

### 3. 运行效果

```
┌────────────────────────────┐
│                            │
│     《Minecraft Reborn》    │  ← title（居中）
│                            │
│    ┌────────────────┐     │
│    │   开始游戏      │     │  ← 按钮（垂直居中，等间距）
│    └────────────────┘     │
│    ┌────────────────┐     │
│    │    选项        │     │
│    └────────────────┘     │
│    ┌────────────────┐     │
│    │    退出        │     │
│    └────────────────┘     │
│                            │
│         v1.0.0           │  ← footer（底部）
└────────────────────────────┘
```

> ✨ 无需手动计算任何坐标！窗口大小变化时，按钮自动保持居中+等间距。

---

> 🌟 **总结**：完善的布局系统 = 声明式模板语法 + 类型安全C++ API + 响应式状态联动 + 增量性能优化。让开发者专注"布局意图"，而非"像素坐标"。
