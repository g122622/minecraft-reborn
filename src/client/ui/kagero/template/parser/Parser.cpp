#include "Parser.hpp"
#include <algorithm>

namespace mc::client::ui::kagero::template::parser {

Parser::Parser(const std::vector<Token>& tokens, const TemplateConfig& config)
    : m_tokens(tokens)
    , m_config(config)
    , m_current(0) {
}

Parser::Parser(const Lexer& lexer, const TemplateConfig& config)
    : m_tokens(lexer.tokens())
    , m_config(config)
    , m_current(0) {
}

std::unique_ptr<DocumentNode> Parser::parse() {
    m_errors.clear();
    m_current = 0;

    auto document = std::make_unique<DocumentNode>();
    document->range.start = currentLocation();

    // 跳过前导空白
    skipWhitespaceAndNewlines();

    // 解析根元素
    while (!isAtEnd()) {
        if (check(TokenType::OpenTag)) {
            auto element = parseElement();
            if (element) {
                document->children.push_back(std::move(element));
            }
        } else if (check(TokenType::OpenComment) || check(TokenType::Comment)) {
            // 跳过注释
            auto comment = parseComment();
            if (comment) {
                document->children.push_back(std::move(comment));
            }
        } else if (check(TokenType::Text)) {
            // 跳过文本
            consume();
        } else {
            // 意外的Token
            addError(TemplateErrorType::UnexpectedToken,
                     "Expected element or comment, got " + String(tokenTypeName(current().type)));
            synchronize();
        }

        skipWhitespaceAndNewlines();
    }

    document->range.end = SourceLocation(m_tokens.back().location.line,
                                          m_tokens.back().location.column + 1,
                                          m_tokens.back().location.offset + 1);

    return document;
}

const Token& Parser::current() const {
    if (m_current < m_tokens.size()) {
        return m_tokens[m_current];
    }
    static const Token eofToken(TokenType::EndOfFile);
    return eofToken;
}

const Token& Parser::peek() const {
    if (m_current + 1 < m_tokens.size()) {
        return m_tokens[m_current + 1];
    }
    static const Token eofToken(TokenType::EndOfFile);
    return eofToken;
}

Token Parser::consume() {
    Token token = current();
    if (!isAtEnd()) {
        ++m_current;
    }
    return token;
}

bool Parser::check(TokenType type) const {
    return current().type == type;
}

bool Parser::check(TokenType type, const String& value) const {
    return current().is(type, value);
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        consume();
        return true;
    }
    return false;
}

bool Parser::match(TokenType type, const String& value) {
    if (check(type, value)) {
        consume();
        return true;
    }
    return false;
}

bool Parser::expect(TokenType type) {
    if (check(type)) {
        consume();
        return true;
    }
    addError(TemplateErrorType::UnexpectedToken,
             "Expected " + String(tokenTypeName(type)) +
             ", got " + String(tokenTypeName(current().type)),
             current());
    return false;
}

bool Parser::expect(TokenType type, const String& value) {
    if (check(type, value)) {
        consume();
        return true;
    }
    addError(TemplateErrorType::UnexpectedToken,
             "Expected '" + value + "', got '" + current().value + "'",
             current());
    return false;
}

Token Parser::expectIdentifier(const String& context) {
    if (!check(TokenType::Identifier)) {
        String msg = "Expected identifier";
        if (!context.empty()) {
            msg += " " + context;
        }
        addError(TemplateErrorType::UnexpectedToken, msg, current());
        return Token(TokenType::Error);
    }
    return consume();
}

Token Parser::expectStringLiteral(const String& context) {
    if (!check(TokenType::StringLiteral)) {
        String msg = "Expected string literal";
        if (!context.empty()) {
            msg += " " + context;
        }
        addError(TemplateErrorType::UnexpectedToken, msg, current());
        return Token(TokenType::Error);
    }
    return consume();
}

void Parser::synchronize() {
    // 跳过Token直到找到一个有效的同步点
    while (!isAtEnd()) {
        switch (current().type) {
            case TokenType::OpenTag:
            case TokenType::OpenCloseTag:
            case TokenType::EndOfFile:
                return;
            default:
                consume();
                break;
        }
    }
}

void Parser::skipWhitespaceAndNewlines() {
    while (check(TokenType::Whitespace) || check(TokenType::Newline)) {
        consume();
    }
}

std::unique_ptr<DocumentNode> Parser::parseDocument() {
    return parse(); // 主解析方法
}

std::unique_ptr<ElementNode> Parser::parseElement() {
    SourceLocation startLoc = currentLocation();

    // 开始标签
    if (!expect(TokenType::OpenTag)) {
        return nullptr;
    }

    // 标签名
    Token nameToken = expectIdentifier("for tag name");
    if (nameToken.type == TokenType::Error) {
        return nullptr;
    }

    String tagName = nameToken.value;

    // 验证标签名
    if (!isValidTagName(tagName)) {
        if (m_config.strictMode) {
            addError(TemplateErrorType::UnknownTag,
                     "Unknown tag: <" + tagName + ">", nameToken);
            // 非严格模式下继续解析
        }
    }

    // 创建元素节点
    NodeType nodeType = getNodeTypeFromTagName(tagName);
    auto element = std::make_unique<ElementNode>(nodeType);
    element->tagName = tagName;
    element->range.start = startLoc;

    // 解析属性
    parseAttributes(*element);

    // 自关闭标签
    if (match(TokenType::SelfCloseTag)) {
        element->range.end = currentLocation();
        validateElement(*element);
        return element;
    }

    // 结束标签
    if (!expect(TokenType::CloseTag)) {
        return nullptr;
    }

    // 解析内容
    parseContent(*element);

    // 结束标签
    if (!parseEndTag(tagName)) {
        // 尝试恢复：不返回nullptr，继续返回部分解析的元素
    }

    element->range.end = currentLocation();

    // 分类属性
    element->categorizeAttributes();

    // 提取指令
    element->loop = extractLoopInfo(*element);
    element->condition = extractConditionInfo(*element);

    // 验证元素
    validateElement(*element);

    return element;
}

String Parser::parseStartTag(bool isOpenTag) {
    (void)isOpenTag; // 未使用

    if (!expect(TokenType::OpenTag)) {
        return "";
    }

    Token nameToken = expectIdentifier("for tag name");
    if (nameToken.type == TokenType::Error) {
        return "";
    }

    return nameToken.value;
}

void Parser::parseAttributes(ElementNode& element) {
    skipWhitespaceAndNewlines();

    while (!isAtEnd() &&
           !check(TokenType::CloseTag) &&
           !check(TokenType::SelfCloseTag)) {

        Attribute attr = parseAttribute();
        if (!attr.name.empty()) {
            // 检查重复属性
            if (element.hasAttribute(attr.name)) {
                // 警告但不报错，后定义的覆盖前面的
            }
            element.addAttribute(attr);
        }

        skipWhitespaceAndNewlines();
    }
}

Attribute Parser::parseAttribute() {
    SourceLocation attrLoc = currentLocation();

    // 属性名（可能带 bind: 或 on: 前缀）
    Token nameToken = parseAttributeName();
    if (nameToken.type == TokenType::Error || nameToken.value.empty()) {
        return Attribute();
    }

    String attrName = nameToken.value;

    // 检查是否是特殊前缀
    bool isBinding = false;
    bool isEvent = false;
    String baseName = attrName;

    static const String BIND_PREFIX = "bind:";
    static const String ON_PREFIX = "on:";

    if (attrName.size() > BIND_PREFIX.size() &&
        attrName.substr(0, BIND_PREFIX.size()) == BIND_PREFIX) {
        isBinding = true;
        baseName = attrName.substr(BIND_PREFIX.size());
    } else if (attrName.size() > ON_PREFIX.size() &&
               attrName.substr(0, ON_PREFIX.size()) == ON_PREFIX) {
        isEvent = true;
        baseName = attrName.substr(ON_PREFIX.size());
    }

    // 期望等号
    if (!expect(TokenType::Equals)) {
        return Attribute();
    }

    // 属性值
    Token valueToken = parseAttributeValue();
    if (valueToken.type == TokenType::Error) {
        return Attribute();
    }

    // 创建属性
    Attribute attr;
    attr.name = attrName;
    attr.rawValue = valueToken.value;
    attr.location = attrLoc;

    if (isBinding) {
        attr = Attribute::createBinding(attrName, valueToken.value, attrLoc);
    } else if (isEvent) {
        attr = Attribute::createEvent(attrName, valueToken.value, attrLoc);
    } else {
        attr = Attribute::createStatic(attrName, valueToken.value, attrLoc);
    }

    return attr;
}

Token Parser::parseAttributeName() {
    // 属性名可以是 identifier 或 identifier:identifier (bind:xxx, on:xxx)
    Token firstPart = expectIdentifier();
    if (firstPart.type == TokenType::Error) {
        return firstPart;
    }

    String name = firstPart.value;

    // 检查冒号
    if (check(TokenType::Colon)) {
        consume(); // 消费冒号

        Token secondPart = expectIdentifier();
        if (secondPart.type == TokenType::Error) {
            return secondPart;
        }

        name += ":" + secondPart.value;
    }

    Token result(TokenType::Identifier, name, firstPart.location);
    return result;
}

Token Parser::parseAttributeValue() {
    skipWhitespaceAndNewlines();

    // 字符串字面量
    if (check(TokenType::StringLiteral)) {
        return consume();
    }

    // 数字字面量（非标准但允许）
    if (check(TokenType::NumberLiteral)) {
        Token numToken = consume();
        // 转换为字符串
        return Token(TokenType::StringLiteral, numToken.value, numToken.location);
    }

    // 标识符（无引号属性值，非标准但允许）
    if (check(TokenType::Identifier)) {
        Token idToken = consume();
        return Token(TokenType::StringLiteral, idToken.value, idToken.location);
    }

    addError(TemplateErrorType::UnexpectedToken,
             "Expected attribute value (string, number, or identifier)",
             current());
    return Token(TokenType::Error);
}

void Parser::parseContent(ElementNode& parent) {
    while (!isAtEnd() && !check(TokenType::OpenCloseTag)) {
        skipWhitespaceAndNewlines();

        if (isAtEnd() || check(TokenType::OpenCloseTag)) {
            break;
        }

        if (check(TokenType::OpenTag)) {
            // 子元素
            auto child = parseElement();
            if (child) {
                parent.children.push_back(std::move(child));
            }
        } else if (check(TokenType::OpenComment) || check(TokenType::Comment)) {
            // 注释
            auto comment = parseComment();
            if (comment) {
                parent.children.push_back(std::move(comment));
            }
        } else if (check(TokenType::Text)) {
            // 文本
            auto text = parseText();
            if (text && !text->isWhitespace) {
                parent.children.push_back(std::move(text));
            }
        } else {
            // 意外的Token，尝试恢复
            addError(TemplateErrorType::UnexpectedToken,
                     "Unexpected token in element content: " + String(tokenTypeName(current().type)),
                     current());
            consume();
        }

        skipWhitespaceAndNewlines();
    }
}

std::unique_ptr<TextNode> Parser::parseText() {
    if (!check(TokenType::Text)) {
        return nullptr;
    }

    Token textToken = consume();
    auto node = std::make_unique<TextNode>();
    node->text = textToken.value;
    node->range = SourceRange(textToken.location, textToken.location);

    // 检查是否全是空白
    node->isWhitespace = std::all_of(textToken.value.begin(), textToken.value.end(),
                                     [](char c) { return Lexer::isWhitespace(c) || c == '\n' || c == '\r'; });

    return node;
}

std::unique_ptr<CommentNode> Parser::parseComment() {
    // 跳过注释开始标记
    if (check(TokenType::OpenComment)) {
        consume();
    }

    // 获取注释内容
    String content;
    if (check(TokenType::Comment)) {
        Token commentToken = consume();
        content = commentToken.value;
    }

    // 跳过注释结束标记
    if (check(TokenType::CloseComment)) {
        consume();
    }

    auto node = std::make_unique<CommentNode>();
    node->text = content;
    return node;
}

bool Parser::parseEndTag(const String& expectedTagName) {
    if (!check(TokenType::OpenCloseTag)) {
        addError(TemplateErrorType::MissingClosingTag,
                 "Expected closing tag </" + expectedTagName + ">",
                 current());
        return false;
    }

    consume(); // 消费 </

    Token nameToken = expectIdentifier("for closing tag name");
    if (nameToken.type == TokenType::Error) {
        return false;
    }

    if (nameToken.value != expectedTagName) {
        addError(TemplateErrorType::MismatchedClosingTag,
                 "Mismatched closing tag: expected </" + expectedTagName +
                 "> but got </" + nameToken.value + ">",
                 nameToken);
        // 尝试恢复
    }

    if (!expect(TokenType::CloseTag)) {
        return false;
    }

    return true;
}

void Parser::validateElement(ElementNode& element) {
    // 验证ID唯一性（如果有）
    if (!element.id.empty()) {
        // TODO: 检查ID唯一性（需要父文档）
    }

    // 验证所有属性
    for (const auto& [name, attr] : element.attributes) {
        validateAttribute(attr, element);
    }
}

void Parser::validateAttribute(const Attribute& attr, const ElementNode& element) {
    // 验证属性名
    if (!isValidAttributeName(attr.name)) {
        addError(TemplateErrorType::InvalidAttributeName,
                 "Invalid attribute name: " + attr.name,
                 attr.location);
        return;
    }

    // 验证绑定路径
    if (attr.isBinding()) {
        if (m_config.validateBindingPaths && attr.binding.has_value()) {
            validateBindingPath(attr.binding->path, attr.location);
        }
    }

    // 验证回调名称
    if (attr.isEvent()) {
        if (m_config.validateCallbackNames) {
            validateCallbackName(attr.callbackName, attr.location);
        }
    }

    // 检查内联表达式
    if (m_config.strictMode && !isInlineExpressionAllowed()) {
        // 检查绑定值中是否有表达式
        if (attr.isBinding() && !attr.rawValue.empty()) {
            // 检测常见的表达式模式
            static const String forbiddenPatterns[] = {
                "{{", "}}", "{%", "%}", "${", "+", "-", "*", "/", "==",
                "!=", "<=", ">=", "&&", "||", "?:", "=>"
            };

            for (const auto& pattern : forbiddenPatterns) {
                if (attr.rawValue.find(pattern) != String::npos) {
                    addError(TemplateErrorType::InlineExpressionNotAllowed,
                             "Inline expressions are not allowed in strict mode. " +
                             "Found pattern '" + pattern + "' in binding: " + attr.rawValue,
                             attr.location);
                    break;
                }
            }
        }
    }
}

void Parser::validateBindingPath(const String& path, const SourceLocation& loc) {
    if (!isValidBindingPath(path)) {
        addError(TemplateErrorType::InvalidBindingPath,
                 "Invalid binding path: '" + path + "'",
                 loc);
    }
}

void Parser::validateCallbackName(const String& name, const SourceLocation& loc) {
    if (!isValidCallbackName(name)) {
        addError(TemplateErrorType::InvalidCallbackName,
                 "Invalid callback name: '" + name + "'. Must be a valid identifier.",
                 loc);
    }
}

bool Parser::isInlineExpressionAllowed() const {
    if (m_config.strictMode) {
        return false;
    }
    return m_config.allowInlineExpression;
}

std::optional<LoopInfo> Parser::extractLoopInfo(const ElementNode& element) {
    // 查找 bind:items 属性
    auto it = element.attributes.find("bind:items");
    if (it == element.attributes.end()) {
        return std::nullopt;
    }

    const Attribute& attr = it->second;
    if (!attr.binding.has_value()) {
        return std::nullopt;
    }

    LoopInfo info;
    info.collectionPath = attr.binding->path;
    info.location = attr.location;

    // 从子元素中推断循环变量名
    // 例如，如果有 bind:item="$slot.item"，则循环变量名为 "slot"
    for (const auto& [name, childAttr] : element.attributes) {
        if (name.find("bind:") == 0 && childAttr.binding.has_value() &&
            childAttr.binding->isLoopVariable) {
            info.itemVarName = childAttr.binding->loopVarName;
            break;
        }
    }

    // 如果没有找到循环变量，使用默认名 "item"
    if (info.itemVarName.empty()) {
        info.itemVarName = "item";
    }

    return info;
}

std::optional<ConditionInfo> Parser::extractConditionInfo(const ElementNode& element) {
    // 查找 bind:visible 属性
    auto it = element.attributes.find("bind:visible");
    if (it == element.attributes.end()) {
        return std::nullopt;
    }

    const Attribute& attr = it->second;
    if (!attr.binding.has_value()) {
        return std::nullopt;
    }

    ConditionInfo info;
    String path = attr.binding->path;

    // 检查是否有取反
    if (!path.empty() && path[0] == '!') {
        info.negate = true;
        path = path.substr(1);
        // 去除可能的空格
        size_t start = path.find_first_not_of(" \t");
        if (start != String::npos) {
            path = path.substr(start);
        }
    }

    info.booleanPath = path;
    info.location = attr.location;

    return info;
}

void Parser::addError(TemplateErrorType type, const String& message,
                      const SourceLocation& loc) {
    m_errors.emplace_back(type, message, loc);
}

void Parser::addError(TemplateErrorType type, const String& message, const Token& token) {
    m_errors.emplace_back(type, message, token.location);
}

bool Parser::isAtEnd() const {
    return m_current >= m_tokens.size() || current().type == TokenType::EndOfFile;
}

SourceLocation Parser::currentLocation() const {
    return current().location;
}

bool Parser::isValidTagName(const String& name) const {
    return ast::isValidWidgetTag(name);
}

} // namespace mc::client::ui::kagero::template::parser
