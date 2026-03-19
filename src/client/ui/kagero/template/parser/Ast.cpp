#include "Ast.hpp"
#include <algorithm>
#include <cctype>
#include <regex>

namespace mc::client::ui::kagero::template::ast {

// ========== BindingInfo ==========

BindingInfo BindingInfo::parse(const String& value) {
    BindingInfo info;

    if (value.empty()) {
        return info;
    }

    // 检查是否是循环变量引用 ($varName.property 或 $varName)
    if (value.size() > 1 && value[0] == '$') {
        info.isLoopVariable = true;

        // 找到变量名和属性
        size_t dotPos = value.find('.');
        if (dotPos != String::npos) {
            // $slot.item 格式
            info.loopVarName = value.substr(1, dotPos - 1);
            info.property = value.substr(dotPos + 1);
            info.path = value; // 保持原始路径
        } else {
            // $slot 格式
            info.loopVarName = value.substr(1);
            info.property = "";
            info.path = value;
        }
    } else {
        // 普通路径绑定 (player.inventory.main)
        info.path = value;
        info.isLoopVariable = false;
    }

    return info;
}

// ========== Attribute ==========

Attribute Attribute::createStatic(const String& name, const String& value,
                                   const SourceLocation& loc) {
    Attribute attr;
    attr.name = name;
    attr.rawValue = value;
    attr.location = loc;
    attr.type = AttributeType::Static;

    // 尝试解析值类型
    // 首先尝试布尔值
    if (value == "true") {
        attr.value = true;
    } else if (value == "false") {
        attr.value = false;
    } else {
        // 尝试整数
        try {
            size_t pos;
            i32 intVal = std::stoi(value, &pos);
            if (pos == value.size()) {
                attr.value = intVal;
            } else {
                // 不是纯整数，当作字符串
                attr.value = value;
            }
        } catch (...) {
            // 尝试浮点数
            try {
                size_t pos;
                f32 floatVal = std::stof(value, &pos);
                if (pos == value.size()) {
                    attr.value = floatVal;
                } else {
                    attr.value = value;
                }
            } catch (...) {
                // 作为字符串
                attr.value = value;
            }
        }
    }

    return attr;
}

Attribute Attribute::createBinding(const String& name, const String& bindingPath,
                                    const SourceLocation& loc) {
    Attribute attr;
    attr.name = name;
    attr.rawValue = bindingPath;
    attr.location = loc;
    attr.type = AttributeType::Binding;
    attr.binding = BindingInfo::parse(bindingPath);
    attr.value = bindingPath; // 保持原始路径作为值
    return attr;
}

Attribute Attribute::createEvent(const String& name, const String& callbackName,
                                  const SourceLocation& loc) {
    Attribute attr;
    attr.name = name;
    attr.rawValue = callbackName;
    attr.location = loc;
    attr.type = AttributeType::Event;
    attr.callbackName = callbackName;
    attr.value = callbackName;
    return attr;
}

String Attribute::baseName() const {
    // 去除 bind: 和 on: 前缀
    static const String bindPrefix = "bind:";
    static const String onPrefix = "on:";

    if (name.size() > bindPrefix.size() &&
        name.substr(0, bindPrefix.size()) == bindPrefix) {
        return name.substr(bindPrefix.size());
    }

    if (name.size() > onPrefix.size() &&
        name.substr(0, onPrefix.size()) == onPrefix) {
        return name.substr(onPrefix.size());
    }

    return name;
}

// ========== ElementNode ==========

void ElementNode::addAttribute(const Attribute& attr) {
    attributes[attr.name] = attr;
}

const Attribute* ElementNode::getAttribute(const String& name) const {
    auto it = attributes.find(name);
    return it != attributes.end() ? &it->second : nullptr;
}

bool ElementNode::hasAttribute(const String& name) const {
    return attributes.find(name) != attributes.end();
}

void ElementNode::categorizeAttributes() {
    staticAttrs.clear();
    bindingAttrs.clear();
    eventAttrs.clear();

    for (const auto& [name, attr] : attributes) {
        switch (attr.type) {
            case AttributeType::Static:
                staticAttrs.push_back(attr);
                break;
            case AttributeType::Binding:
                bindingAttrs.push_back(attr);
                break;
            case AttributeType::Event:
                eventAttrs.push_back(attr);
                break;
        }
    }
}

std::unique_ptr<Node> ElementNode::clone() const {
    auto node = std::make_unique<ElementNode>(type);
    node->tagName = tagName;
    node->id = id;
    node->classes = classes;
    node->attributes = attributes;
    node->staticAttrs = staticAttrs;
    node->bindingAttrs = bindingAttrs;
    node->eventAttrs = eventAttrs;
    node->loop = loop;
    node->condition = condition;
    node->range = range;

    // 深拷贝子节点
    for (const auto& child : children) {
        node->children.push_back(child->clone());
    }

    return node;
}

// ========== DocumentNode ==========

ElementNode* DocumentNode::rootElement() {
    for (auto& child : children) {
        if (auto* elem = dynamic_cast<ElementNode*>(child.get())) {
            return elem;
        }
    }
    return nullptr;
}

const ElementNode* DocumentNode::rootElement() const {
    for (const auto& child : children) {
        if (const auto* elem = dynamic_cast<const ElementNode*>(child.get())) {
            return elem;
        }
    }
    return nullptr;
}

std::unique_ptr<Node> DocumentNode::clone() const {
    auto node = std::make_unique<DocumentNode>();
    node->sourcePath = sourcePath;
    node->version = version;
    node->range = range;

    for (const auto& child : children) {
        node->children.push_back(child->clone());
    }

    return node;
}

// ========== 工具函数 ==========

NodeType getNodeTypeFromTagName(const String& tagName) {
    static const std::map<String, NodeType> tagMap = {
        {"screen", NodeType::Screen},
        {"widget", NodeType::Widget},
        {"button", NodeType::Button},
        {"text", NodeType::Text},
        {"textfield", NodeType::TextField},
        {"slider", NodeType::Slider},
        {"checkbox", NodeType::Checkbox},
        {"image", NodeType::Image},
        {"grid", NodeType::Grid},
        {"slot", NodeType::Slot},
        {"viewport3d", NodeType::Viewport3D},
        {"scrollable", NodeType::Scrollable},
        {"list", NodeType::List},
        {"style", NodeType::Style}
    };

    // 转换为小写
    String lowerName = tagName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    auto it = tagMap.find(lowerName);
    return it != tagMap.end() ? it->second : NodeType::Widget;
}

bool isValidWidgetTag(const String& tagName) {
    static const std::set<String> validTags = {
        "screen", "widget", "button", "text", "textfield",
        "slider", "checkbox", "image", "grid", "slot",
        "viewport3d", "scrollable", "list", "style"
    };

    String lowerName = tagName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    return validTags.count(lowerName) > 0;
}

bool isValidAttributeName(const String& name) {
    if (name.empty()) return false;

    // 属性名必须以字母或下划线开头
    // 可以包含字母、数字、下划线、连字符、冒号
    // 特殊前缀: bind:, on:

    static const std::regex validPattern("^[a-zA-Z_][a-zA-Z0-9_\\-:]*$");
    return std::regex_match(name, validPattern);
}

bool isValidBindingPath(const String& path) {
    if (path.empty()) return false;

    // 检查是否是循环变量 ($varName 或 $varName.property)
    if (path.size() > 1 && path[0] == '$') {
        // $varName 格式
        String rest = path.substr(1);
        if (rest.empty()) return false;

        size_t dotPos = rest.find('.');
        if (dotPos == String::npos) {
            // 只有变量名
            return std::regex_match(rest, std::regex("^[a-zA-Z_][a-zA-Z0-9_]*$"));
        }

        // $varName.property 格式
        String varName = rest.substr(0, dotPos);
        String property = rest.substr(dotPos + 1);

        return std::regex_match(varName, std::regex("^[a-zA-Z_][a-zA-Z0-9_]*$")) &&
               isValidBindingPath(property);
    }

    // 普通路径: identifier.identifier...
    // 支持数组索引: array[0], items[index]
    static const std::regex pathPattern(
        "^[a-zA-Z_][a-zA-Z0-9_]*(\\.[a-zA-Z_][a-zA-Z0-9_]*|\\[[0-9]+\\]|\\[[a-zA-Z_][a-zA-Z0-9_]*\\])*$"
    );
    return std::regex_match(path, pathPattern);
}

bool isValidCallbackName(const String& name) {
    if (name.empty()) return false;

    // 回调名称必须是有效的C++标识符
    static const std::regex identifierPattern("^[a-zA-Z_][a-zA-Z0-9_]*$");
    return std::regex_match(name, identifierPattern);
}

} // namespace mc::client::ui::kagero::template::ast
