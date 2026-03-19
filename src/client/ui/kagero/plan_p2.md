# 🔄 第二阶段：Kagero 模板系统（修订版）

> 核心原则：**声明式模板 + C++ 逻辑注入**，零内联脚本，类型安全，双向绑定

---

## 📐 模板语法规范（严格声明式）

### ✅ 允许的语法特性

```xml
<!-- 1. 基础标签结构 -->
<widget id="uniqueId" class="style1 style2" pos="x,y" size="w,h">
    <child/>
</widget>

<!-- 2. 属性绑定（单向：状态 → UI） -->
<text bind:text="player.name"/>
<slot bind:item="$slot.item" bind:count="$slot.count"/>

<!-- 3. 事件绑定（回调：UI → C++） -->
<button on:click="onStartGame" on:hover="onButtonHover"/>

<!-- 4. 循环渲染（仅数据驱动，无逻辑） -->
<grid bind:items="inventory.slots">
    <slot bind:item="$item.id"/>  <!-- $item 是循环变量 -->
</grid>

<!-- 5. 条件渲染（仅布尔表达式） -->
<tooltip bind:visible="player.isSneaking">
    <text>按住Shift潜行</text>
</tooltip>

<!-- 6. 样式引用（外部或内联） -->
<style ref="default-theme"/>
<!-- 或 -->
<style>
    button { background: "gui/button.png"; }
    button:hover { border: 2px solid #FFD700; }
</style>
```

### ❌ 明确禁止的特性

```xml
<!-- 🚫 不允许任何脚本/表达式执行 -->
<script>...</script>                    <!-- 直接拒绝 -->
{{ player.health + 10 }}                <!-- 不允许算术 -->
{% if player.level > 10 %}...{% endif %} <!-- 不允许复杂逻辑 -->
on:click="player.health -= 1"           <!-- 不允许内联语句 -->
bind:style="player.isVIP ? 'gold' : 'normal'"  <!-- 不允许三元表达式 -->

<!-- 🚫 不允许动态标签名/属性名 -->
<{{ dynamicTag }}/>                     <!-- 拒绝 -->
<widget {{ dynamicAttr }}="value"/>     <!-- 拒绝 -->
```

> 💡 **设计哲学**：模板只描述"是什么"，不描述"怎么做"。所有逻辑、计算、状态变更都在C++层完成。

---

## 🏗️ 修订后的目录结构

```
src/client/ui/kagero/template/
├── core/
│   ├── Template.hpp                  # 模板入口类
│   ├── Template.cpp
│   ├── TemplateConfig.hpp            # 解析配置（严格模式开关）
│   └── TemplateError.hpp             # 错误类型定义
├── parser/
│   ├── Lexer.hpp                     # 词法分析器（手写的，零依赖）
│   ├── Lexer.cpp
│   ├── Parser.hpp                    # 递归下降语法分析器
│   ├── Parser.cpp
│   ├── Ast.hpp                       # AST节点定义
│   ├── Ast.cpp
│   └── AstVisitor.hpp                # AST遍历接口
├── binder/
│   ├── BindingContext.hpp            # 绑定上下文（连接StateStore）
│   ├── BindingContext.cpp
│   ├── PropertyBinder.hpp            # 属性绑定器（单向）
│   ├── PropertyBinder.cpp
│   ├── EventBinder.hpp               # 事件绑定器（回调注册）
│   ├── EventBinder.cpp
│   ├── LoopBinder.hpp                # 循环渲染绑定器
│   └── LoopBinder.cpp
├── compiler/
│   ├── TemplateCompiler.hpp          # 模板→可执行描述
│   ├── TemplateCompiler.cpp
│   ├── CompiledTemplate.hpp          # 编译结果（轻量、可缓存）
│   ├── CompiledTemplate.cpp
│   └── TemplateCache.hpp             # 编译结果缓存
├── runtime/
│   ├── TemplateInstance.hpp          # 运行时实例（绑定状态）
│   ├── TemplateInstance.cpp
│   ├── UpdateScheduler.hpp           # 增量更新调度器
│   └── UpdateScheduler.cpp
└── bindings/
    ├── BuiltinWidgets.hpp            # 内置Widget注册表
    ├── BuiltinWidgets.cpp
    ├── BuiltinEvents.hpp             # 内置事件类型
    └── BuiltinStyles.hpp             # 内置样式规则
```

---

## 🔍 核心组件设计

### 1. AST 节点（严格类型化）

```cpp
namespace mc::client::ui::kagero::tpl::ast {

enum class NodeType {
    Screen, Widget, Grid, Slot, Viewport3D, Button, Text, Image, Style,
    BindingAttr, EventAttr, LoopDirective, ConditionDirective  // 显式区分
};

struct Attribute {
    String name;          // "pos", "bind:item", "on:click"
    String value;         // "8,84", "player.inventory.main", "onStartGame"
    
    // 属性类型自动推断（解析时）
    enum class Type { Static, Binding, Event, Expression } type;
    
    // 如果是绑定，解析出路径
    struct BindingInfo {
        String path;              // "player.inventory.main"
        bool isLoopVariable;      // 是否是 $slot.item 这种
        String loopVarName;       // "slot"
    };
    std::optional<BindingInfo> binding;
};

struct WidgetNode {
    String tagName;                    // "button", "slot"
    String id;                         // 可选，用于查找
    std::vector<String> classes;       // 样式类
    std::map<String, Attribute> attrs; // 所有属性
    
    // 显式分离的绑定/事件（方便后续处理）
    std::vector<Attribute> staticAttrs;
    std::vector<Attribute> bindingAttrs;   // bind:xxx
    std::vector<Attribute> eventAttrs;     // on:xxx
    
    std::vector<std::unique_ptr<WidgetNode>> children;
    
    // 循环/条件指令（如果有）
    struct LoopInfo {
        String collectionPath;  // "inventory.slots"
        String itemVarName;     // "$slot" → "slot"
    };
    std::optional<LoopInfo> loop;
    
    struct ConditionInfo {
        String booleanPath;  // "player.isSneaking"
    };
    std::optional<ConditionInfo> condition;
};

} // namespace ast
```

### 2. 绑定上下文（连接模板与C++状态）

```cpp
namespace mc::client::ui::kagero::tpl::binder {

// 绑定上下文：模板实例与StateStore/EventBus的桥梁
class BindingContext {
public:
    BindingContext(state::StateStore& store, event::EventBus& bus);
    
    // 🔹 注册C++变量供模板绑定（单向：状态→UI）
    template<typename T>
    void expose(const String& path, T* ptr, 
                std::function<void(const T&)> onUpdate = nullptr);
    
    // 🔹 注册C++回调供模板事件调用（单向：UI→逻辑）
    void exposeCallback(const String& name, 
                       std::function<void(widget::Widget*, const event::Event&)> cb);
    
    // 🔹 解析绑定路径并获取值（内部使用）
    std::optional<Value> resolveBinding(const String& path, 
                                       const String& loopVar = "",
                                       const Value& loopValue = {});
    
    // 🔹 触发双向绑定更新（当C++状态变更时）
    void notifyChange(const String& path);
    
private:
    state::StateStore& m_store;
    event::EventBus& m_eventBus;
    
    // 注册表：路径 → 变量指针+更新回调
    struct ExposedVar {
        std::any ptr;  // T*
        std::function<void(const std::any&)> onUpdate;
        String typeId; // 用于类型安全检查
    };
    std::map<String, ExposedVar> m_exposedVars;
    
    // 回调注册表：函数名 → C++函数
    std::map<String, std::function<void(widget::Widget*, const event::Event&)>> 
        m_callbacks;
};

} // namespace binder
```

### 3. 模板编译器（模板 → 可执行描述）

```cpp
namespace mc::client::ui::kagero::tpl::compiler {

// 编译结果：轻量、可序列化、可缓存
class CompiledTemplate {
public:
    // 实例化为实际Widget树
    std::unique_ptr<widget::Widget> instantiate(
        binder::BindingContext& ctx,
        widget::Widget* parent = nullptr);
    
    // 获取所有需要监听的状态路径（用于增量更新）
    const std::vector<String>& getWatchedPaths() const;
    
    // 获取所有注册的事件处理器
    const std::vector<String>& getRegisteredEvents() const;
    
private:
    friend class TemplateCompiler;
    
    // AST根节点（编译后保留用于调试/热重载）
    std::unique_ptr<ast::WidgetNode> m_astRoot;
    
    // 预解析的绑定信息（加速运行时）
    struct BindingPlan {
        String widgetPath;      // "screen.grid.slot"
        String statePath;       // "player.inventory.main[0].item"
        bool isLoopBinding;     // 是否是循环内的绑定
    };
    std::vector<BindingPlan> m_bindingPlans;
    
    // 事件处理器映射
    struct EventPlan {
        String widgetPath;
        String eventName;       // "click", "hover"
        String callbackName;    // "onStartGame"
    };
    std::vector<EventPlan> m_eventPlans;
};

// 编译器主类
class TemplateCompiler {
public:
    TemplateCompiler();
    
    // 编译模板源码
    std::unique_ptr<CompiledTemplate> compile(const String& source, 
                                             const String& sourcePath = "");
    
    // 从文件编译
    std::unique_ptr<CompiledTemplate> compileFile(const String& filePath);
    
    // 设置严格模式（默认开启，禁止所有动态特性）
    void setStrictMode(bool enabled);
    
private:
    parser::Lexer m_lexer;
    parser::Parser m_parser;
    
    // 语义检查：确保无内联代码、绑定路径合法等
    bool validateAst(const ast::WidgetNode* root, std::string& error);
};

} // namespace compiler
```

---

## 🔄 双向绑定实现机制

### 单向绑定（状态 → UI）

```cpp
// C++侧注册变量
BindingContext ctx(stateStore, eventBus);
ctx.expose("player.name", &player.name, [](const String& newName) {
    // 可选：当状态变更时的额外逻辑
    Logger::info("Player name changed to: {}", newName);
});

// 模板中使用
// <text bind:text="player.name"/>

// 内部流程：
// 1. 解析时：识别 bind:text="player.name" → 记录路径
// 2. 实例化时：通过 ctx.resolveBinding("player.name") 获取当前值
// 3. 渲染时：设置 Widget 的 text 属性
// 4. 状态变更时：ctx.notifyChange("player.name") → 触发对应 Widget 重渲染
```

### 事件绑定（UI → C++）

```cpp
// C++侧注册回调
ctx.exposeCallback("toggleRecipeBook", 
    [](widget::Widget* source, const event::Event& evt) {
        auto* btn = dynamic_cast<widget::ButtonWidget*>(source);
        if (btn) {
            // 执行游戏逻辑
            RecipeBookSystem::toggle();
            // 可选：更新相关状态
            stateStore.set("recipeBook.visible", !currentVisible);
        }
    });

// 模板中使用
// <button on:click="toggleRecipeBook()">

// 内部流程：
// 1. 解析时：识别 on:click="toggleRecipeBook" → 记录回调名
// 2. 实例化时：从 ctx 查找回调函数，绑定到 Widget 的点击事件
// 3. 用户点击时：Widget → EventBus → 查找并调用注册的C++回调
```

### 循环渲染绑定

```cpp
// 模板
<grid bind:items="player.inventory.main">
    <slot bind:item="$item.itemId" bind:count="$item.count"/>
</grid>

// C++侧：暴露一个列表
std::vector<ItemSlot> inventory = { ... };
ctx.expose("player.inventory.main", &inventory);

// 内部流程：
// 1. 解析时：识别 bind:items → 标记为循环，记录 $item 变量名
// 2. 实例化时：
//    - 获取 inventory 列表
//    - 为每个元素创建 <slot> 实例
//    - 在子作用域中绑定 $item → 当前元素
// 3. 状态变更时：
//    - 如果列表长度变化 → 增删 Widget
//    - 如果元素内容变化 → 更新对应 Widget
```

---

## 🛡️ 安全与验证机制

### 编译时验证（模板加载阶段）

```cpp
bool TemplateCompiler::validateAst(const ast::WidgetNode* root, std::string& error) {
    // 1. 检查标签白名单
    static const std::set<String> ALLOWED_TAGS = {
        "screen", "button", "text", "slot", "grid", "viewport3d", "image", "style"
    };
    if (!ALLOWED_TAGS.count(root->tagName)) {
        error = fmt::format("Unknown tag: <{}>", root->tagName);
        return false;
    }
    
    // 2. 检查属性白名单
    for (const auto& [name, attr] : root->attrs) {
        if (name.starts_with("on:") && !isValidCallbackName(attr.value)) {
            error = fmt::format("Invalid callback name: {}", attr.value);
            return false;
        }
        if (name.starts_with("bind:") && !isValidPath(attr.value)) {
            error = fmt::format("Invalid binding path: {}", attr.value);
            return false;
        }
    }
    
    // 3. 禁止内联表达式
    if (containsScriptPattern(root)) {
        error = "Inline scripts/expressions are not allowed in Kagero templates";
        return false;
    }
    
    // 4. 递归检查子节点
    for (const auto& child : root->children) {
        if (!validateAst(child.get(), error)) return false;
    }
    
    return true;
}
```

### 运行时保护

```cpp
// BindingContext::resolveBinding 实现
std::optional<Value> BindingContext::resolveBinding(const String& path, 
                                                   const String& loopVar,
                                                   const Value& loopValue) {
    // 1. 处理循环变量
    if (path.starts_with("$" + loopVar + ".")) {
        String subPath = path.substr(loopVar.size() + 2);  // "$slot.item" → "item"
        return loopValue.getProperty(subPath);  // 从循环项中获取
    }
    
    // 2. 查找已暴露的变量
    auto it = m_exposedVars.find(path);
    if (it != m_exposedVars.end()) {
        // 类型安全读取
        return readExposedValue(it->second);
    }
    
    // 3. 尝试从 StateStore 获取（只读）
    if (m_store.has(path)) {
        return m_store.getReadOnly(path);  // 返回副本，防止外部修改
    }
    
    // 4. 路径不存在 → 返回空（模板中可设默认值）
    return std::nullopt;
}
```

---

## 🚀 使用流程示例

### 步骤1：准备C++逻辑层

```cpp
// game_logic.cpp
class InventoryScreenLogic {
public:
    InventoryScreenLogic(Player& player) : m_player(player) {
        // 暴露状态供模板绑定
        m_ctx.expose("player.name", &m_player.name);
        m_ctx.expose("player.inventory.main", &m_player.inventory.main);
        m_ctx.expose("player.inventory.hotbar", &m_player.inventory.hotbar);
        
        // 暴露回调供模板事件调用
        m_ctx.exposeCallback("toggleRecipeBook", 
            [this](auto*, const auto&) { toggleRecipeBook(); });
        m_ctx.exposeCallback("onSlotClick", 
            [this](widget::Widget* src, const auto& evt) { handleSlotClick(src, evt); });
    }
    
    // 创建屏幕
    std::unique_ptr<Screen> createScreen() {
        auto compiled = m_compiler.compileFile("templates/inventory_screen.tpl");
        return compiled->instantiate(m_ctx);
    }
    
private:
    void toggleRecipeBook() { /* ... */ }
    void handleSlotClick(widget::Widget* slot, const event::Event& evt) { /* ... */ }
    
    Player& m_player;
    template::binder::BindingContext m_ctx;
    template::compiler::TemplateCompiler m_compiler;
};
```

### 步骤2：编写声明式模板

```xml
<!-- inventory_screen.tpl -->
<screen id="inventory" size="176,166" title="container.inventory">
    <!-- 物品槽网格：循环渲染 -->
    <grid id="mainInventory" pos="8,84" cols="9" rows="3" slotSize="18"
          bind:items="player.inventory.main">
        <!-- $slot 是循环变量，自动绑定到当前元素 -->
        <slot bind:item="$slot.item" 
              bind:count="$slot.count"
              on:click="onSlotClick"/>
    </grid>
    
    <!-- 按钮：事件绑定 -->
    <button id="recipeBookBtn" 
            pos="104,22" 
            size="20,18"
            on:click="toggleRecipeBook">
        <image src="gui/recipe_button.png"/>
    </button>
    
    <!-- 条件渲染：仅当玩家有物品时显示提示 -->
    <tooltip bind:visible="player.hasSelectedItem">
        <text bind:text="player.selectedItemTooltip"/>
    </tooltip>
</screen>
```

### 步骤3：运行时更新（双向绑定生效）

```cpp
// 游戏循环中
void Game::update(f32 dt) {
    // C++逻辑修改状态
    if (player.pickupItem(newItem)) {
        // 自动触发：所有绑定 "player.inventory.*" 的UI更新
        // 无需手动调用 refresh()！
    }
    
    // UI事件触发C++逻辑
    // 用户点击按钮 → toggleRecipeBook() 被调用 → 可能修改状态 → 再次触发UI更新
    // 形成闭环，但逻辑完全在C++层
}
```

---

## 📦 依赖与集成

### 零外部依赖设计（解析器部分）

```cpp
// Lexer/Parser 完全手写，不依赖 Boost/PEG 等
// 仅使用 C++17 标准库 + 项目基础类型（String, Optional, Variant 等）

// 可选：如果项目已有，可集成
// - fmt::format 用于错误信息
// - nlohmann::json 用于调试导出
// 但核心解析逻辑不依赖它们
```

### CMake 集成

```cmake
# src/client/ui/kagero/template/CMakeLists.txt
add_library(kagero_template STATIC
    core/Template.cpp
    parser/Lexer.cpp
    parser/Parser.cpp
    parser/Ast.cpp
    binder/BindingContext.cpp
    binder/PropertyBinder.cpp
    binder/EventBinder.cpp
    compiler/TemplateCompiler.cpp
    compiler/CompiledTemplate.cpp
    runtime/TemplateInstance.cpp
)

target_link_libraries(kagero_template PRIVATE
    kagero_core      # Widget/StateStore/EventBus 等基础组件
    kagero_widget    # 具体Widget实现
)

# 严格模式：编译时检查更严格
target_compile_definitions(kagero_template PRIVATE 
    KAGERO_TEMPLATE_STRICT=1
)
```

---

## 🎯 与原始方案的关键改进

| 方面 | 原方案 | 修订方案 | 优势 |
|------|--------|----------|------|
| **脚本执行** | 未明确限制 | 编译时+运行时双重禁止 | 安全、可预测、易调试 |
| **绑定语义** | 模糊的 `bind:xxx` | 显式区分 `bind:` / `on:` / `loop` | 类型安全、工具友好 |
| **循环变量** | `$slot.item` 语法 | 显式 `bind:items` + `$var` 作用域 | 避免命名冲突、易理解 |
| **错误处理** | 运行时崩溃 | 编译时验证 + 详细错误定位 | 开发体验提升 |
| **性能** | 每次解析 | 预编译 + 缓存 + 增量更新 | 运行时零解析开销 |
| **类型安全** | 字符串路径 | 可选的强类型绑定接口 | 减少运行时错误 |

---

## 🧪 测试策略建议

```cpp
// template_test.cpp
TEST(TemplateParser, RejectsInlineScript) {
    TemplateCompiler compiler;
    auto result = compiler.compile(R"(
        <button on:click="player.health -= 1">Hack</button>
    )");
    EXPECT_FALSE(result);  // 应该编译失败
    EXPECT_THAT(compiler.lastError(), HasSubstr("not allowed"));
}

TEST(TemplateBinder, TwoWayBinding) {
    StateStore store;
    EventBus bus;
    BindingContext ctx(store, bus);
    
    int health = 100;
    ctx.expose("player.health", &health);
    
    auto compiled = compiler.compile(R"(
        <text id="hp" bind:text="player.health"/>
    )");
    auto widget = compiled->instantiate(ctx);
    
    // 初始渲染
    EXPECT_EQ(widget->find("hp")->asText()->text(), "100");
    
    // C++修改状态 → UI自动更新
    health = 80;
    ctx.notifyChange("player.health");
    EXPECT_EQ(widget->find("hp")->asText()->text(), "80");
}
```

---

> 🌟 **总结**：修订后的模板系统坚持"声明式 + 类型安全 + 零内联代码"原则，通过编译时验证、显式绑定语义、C++ 逻辑注入，既保留了模板的灵活性，又确保了项目的可维护性和安全性。
