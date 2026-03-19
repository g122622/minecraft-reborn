#include <gtest/gtest.h>

// 模板系统核心组件
#include "client/ui/kagero/template/core/TemplateConfig.hpp"
#include "client/ui/kagero/template/core/TemplateError.hpp"
#include "client/ui/kagero/template/parser/Lexer.hpp"
#include "client/ui/kagero/template/parser/Parser.hpp"
#include "client/ui/kagero/template/parser/Ast.hpp"
#include "client/ui/kagero/template/parser/AstVisitor.hpp"

using namespace mc::client::ui::kagero::tpl;
using mc::String;

// ========== Lexer测试 ==========

class LexerTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    parser::Lexer createLexer(const String& source) {
        return parser::Lexer(source, "<test>");
    }
};

TEST_F(LexerTest, EmptySource) {
    auto lexer = createLexer("");
    EXPECT_TRUE(lexer.tokenize());
    EXPECT_TRUE(lexer.tokens().empty() ||
                lexer.tokens().back().type == parser::TokenType::EndOfFile);
}

TEST_F(LexerTest, SimpleTag) {
    auto lexer = createLexer("<screen>");
    EXPECT_TRUE(lexer.tokenize());

    auto& tokens = lexer.tokens();
    ASSERT_GE(tokens.size(), 2);
    EXPECT_EQ(tokens[0].type, parser::TokenType::OpenTag);
    EXPECT_EQ(tokens[1].type, parser::TokenType::Identifier);
    EXPECT_EQ(tokens[1].value, "screen");
}

TEST_F(LexerTest, TagWithAttributes) {
    auto lexer = createLexer(R"(<button id="btn1" pos="10,20">)");
    EXPECT_TRUE(lexer.tokenize());

    auto& tokens = lexer.tokens();

    // 验证Token序列
    bool foundId = false;
    bool foundPos = false;

    for (const auto& token : tokens) {
        if (token.value == "id") foundId = true;
        if (token.value == "pos") foundPos = true;
    }

    EXPECT_TRUE(foundId);
    EXPECT_TRUE(foundPos);
}

TEST_F(LexerTest, BindingAttribute) {
    auto lexer = createLexer(R"(<text bind:text="player.name">)");
    EXPECT_TRUE(lexer.tokenize());

    auto& tokens = lexer.tokens();

    // 查找bind:text
    bool foundBind = false;
    for (const auto& token : tokens) {
        if (token.value == "bind:text") {
            foundBind = true;
            break;
        }
    }
    EXPECT_TRUE(foundBind);
}

TEST_F(LexerTest, EventAttribute) {
    auto lexer = createLexer(R"(<button on:click="onButtonClick">)");
    EXPECT_TRUE(lexer.tokenize());

    auto& tokens = lexer.tokens();

    bool foundEvent = false;
    for (const auto& token : tokens) {
        if (token.value == "on:click") {
            foundEvent = true;
            break;
        }
    }
    EXPECT_TRUE(foundEvent);
}

TEST_F(LexerTest, SelfClosingTag) {
    auto lexer = createLexer("<text/>");
    EXPECT_TRUE(lexer.tokenize());

    auto& tokens = lexer.tokens();
    bool foundSelfClose = false;
    for (const auto& token : tokens) {
        if (token.type == parser::TokenType::SelfCloseTag) {
            foundSelfClose = true;
            break;
        }
    }
    EXPECT_TRUE(foundSelfClose);
}

TEST_F(LexerTest, ClosingTag) {
    auto lexer = createLexer("</screen>");
    EXPECT_TRUE(lexer.tokenize());

    auto& tokens = lexer.tokens();
    ASSERT_GE(tokens.size(), 2);
    EXPECT_EQ(tokens[0].type, parser::TokenType::OpenCloseTag);
    EXPECT_EQ(tokens[1].value, "screen");
}

TEST_F(LexerTest, Comment) {
    auto lexer = createLexer("<!-- This is a comment -->");
    EXPECT_TRUE(lexer.tokenize());

    auto& tokens = lexer.tokens();
    bool foundComment = false;
    for (const auto& token : tokens) {
        if (token.type == parser::TokenType::Comment) {
            foundComment = true;
            break;
        }
    }
    EXPECT_TRUE(foundComment);
}

TEST_F(LexerTest, TextContent) {
    auto lexer = createLexer("Hello World");
    EXPECT_TRUE(lexer.tokenize());

    auto& tokens = lexer.tokens();
    bool foundText = false;
    for (const auto& token : tokens) {
        if (token.type == parser::TokenType::Text) {
            foundText = true;
            EXPECT_EQ(token.value, "Hello World");
            break;
        }
    }
    EXPECT_TRUE(foundText);
}

TEST_F(LexerTest, StringEscaping) {
    auto lexer = createLexer(R"(<text value="Hello\nWorld">)");
    EXPECT_TRUE(lexer.tokenize());

    auto& tokens = lexer.tokens();
    bool foundString = false;
    for (const auto& token : tokens) {
        if (token.type == parser::TokenType::StringLiteral) {
            foundString = true;
            EXPECT_TRUE(token.value.find('\n') != String::npos ||
                       token.value.find("\\n") != String::npos);
            break;
        }
    }
    EXPECT_TRUE(foundString);
}

// ========== Parser测试 ==========

class ParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_config = core::TemplateConfig::defaults();
    }

    std::unique_ptr<ast::DocumentNode> parse(const String& source) {
        parser::Lexer lexer(source, "<test>");
        lexer.tokenize();

        parser::Parser parser(lexer, m_config);
        return parser.parse();
    }

    core::TemplateConfig m_config;
};

TEST_F(ParserTest, SimpleElement) {
    auto doc = parse("<screen/>");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);
    EXPECT_EQ(doc->rootElement()->tagName, "screen");
}

TEST_F(ParserTest, ElementWithId) {
    auto doc = parse(R"(<button id="myButton"/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);
    EXPECT_EQ(doc->rootElement()->id, "myButton");
}

TEST_F(ParserTest, ElementWithStaticAttributes) {
    auto doc = parse(R"(<text pos="10,20" size="100,30"/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* elem = doc->rootElement();
    EXPECT_TRUE(elem->hasAttribute("pos"));
    EXPECT_TRUE(elem->hasAttribute("size"));
}

TEST_F(ParserTest, ElementWithBindingAttribute) {
    auto doc = parse(R"(<text bind:text="player.name"/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* elem = doc->rootElement();
    EXPECT_TRUE(elem->hasAttribute("bind:text"));

    const auto* attr = elem->getAttribute("bind:text");
    ASSERT_NE(attr, nullptr);
    EXPECT_TRUE(attr->isBinding());
    ASSERT_TRUE(attr->binding.has_value());
    EXPECT_EQ(attr->binding->path, "player.name");
}

TEST_F(ParserTest, ElementWithEventAttribute) {
    auto doc = parse(R"(<button on:click="onButtonClick"/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* elem = doc->rootElement();
    EXPECT_TRUE(elem->hasAttribute("on:click"));

    const auto* attr = elem->getAttribute("on:click");
    ASSERT_NE(attr, nullptr);
    EXPECT_TRUE(attr->isEvent());
    EXPECT_EQ(attr->callbackName, "onButtonClick");
}

TEST_F(ParserTest, NestedElements) {
    auto doc = parse(R"(
        <screen>
            <text id="title" text="Hello"/>
            <button id="startBtn" text="Start"/>
        </screen>
    )");

    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);
    EXPECT_EQ(doc->rootElement()->children.size(), 2);
}

TEST_F(ParserTest, TextContent) {
    auto doc = parse(R"(
        <screen>
            Hello World
        </screen>
    )");

    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);
    EXPECT_FALSE(doc->rootElement()->children.empty());
}

TEST_F(ParserTest, CommentParsing) {
    auto doc = parse(R"(
        <screen>
            <!-- This is a comment -->
            <text/>
        </screen>
    )");

    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);
}

TEST_F(ParserTest, AttributeCategorization) {
    auto doc = parse(R"(<text pos="10,20" bind:text="player.name" on:click="onClick"/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* elem = doc->rootElement();
    elem->categorizeAttributes();

    EXPECT_EQ(elem->staticAttrs.size(), 1);
    EXPECT_EQ(elem->bindingAttrs.size(), 1);
    EXPECT_EQ(elem->eventAttrs.size(), 1);
}

// ========== AST测试 ==========

class AstTest : public ::testing::Test {
};

TEST_F(AstTest, NodeTypeNames) {
    EXPECT_STREQ(ast::nodeTypeName(ast::NodeType::Document), "Document");
    EXPECT_STREQ(ast::nodeTypeName(ast::NodeType::Screen), "Screen");
    EXPECT_STREQ(ast::nodeTypeName(ast::NodeType::Button), "Button");
    EXPECT_STREQ(ast::nodeTypeName(ast::NodeType::Text), "Text");
}

TEST_F(AstTest, BindingInfoParse) {
    // 普通路径
    auto info1 = ast::BindingInfo::parse("player.name");
    EXPECT_FALSE(info1.isLoopVariable);
    EXPECT_EQ(info1.path, "player.name");

    // 循环变量
    auto info2 = ast::BindingInfo::parse("$item");
    EXPECT_TRUE(info2.isLoopVariable);
    EXPECT_EQ(info2.loopVarName, "item");

    // 循环变量带属性
    auto info3 = ast::BindingInfo::parse("$slot.item");
    EXPECT_TRUE(info3.isLoopVariable);
    EXPECT_EQ(info3.loopVarName, "slot");
    EXPECT_EQ(info3.property, "item");
}

TEST_F(AstTest, AttributeCreate) {
    auto staticAttr = ast::Attribute::createStatic("pos", "10,20");
    EXPECT_TRUE(staticAttr.isStatic());
    EXPECT_EQ(staticAttr.name, "pos");
    EXPECT_EQ(staticAttr.rawValue, "10,20");

    auto bindingAttr = ast::Attribute::createBinding("bind:text", "player.name");
    EXPECT_TRUE(bindingAttr.isBinding());
    EXPECT_TRUE(bindingAttr.binding.has_value());
    EXPECT_EQ(bindingAttr.binding->path, "player.name");

    auto eventAttr = ast::Attribute::createEvent("on:click", "onButtonClick");
    EXPECT_TRUE(eventAttr.isEvent());
    EXPECT_EQ(eventAttr.callbackName, "onButtonClick");
}

TEST_F(AstTest, ValidTagName) {
    EXPECT_TRUE(ast::isValidWidgetTag("screen"));
    EXPECT_TRUE(ast::isValidWidgetTag("button"));
    EXPECT_TRUE(ast::isValidWidgetTag("text"));
    EXPECT_TRUE(ast::isValidWidgetTag("slot"));
    EXPECT_TRUE(ast::isValidWidgetTag("grid"));
    EXPECT_FALSE(ast::isValidWidgetTag("invalidTag"));
    EXPECT_FALSE(ast::isValidWidgetTag("script"));
}

TEST_F(AstTest, ValidBindingPath) {
    EXPECT_TRUE(ast::isValidBindingPath("player.name"));
    EXPECT_TRUE(ast::isValidBindingPath("player.inventory.slots[0]"));
    EXPECT_TRUE(ast::isValidBindingPath("player.inventory.main"));
    EXPECT_TRUE(ast::isValidBindingPath("$item"));
    EXPECT_TRUE(ast::isValidBindingPath("$slot.item"));
    EXPECT_FALSE(ast::isValidBindingPath("123invalid"));
    EXPECT_FALSE(ast::isValidBindingPath("player..name"));
}

TEST_F(AstTest, ValidCallbackName) {
    EXPECT_TRUE(ast::isValidCallbackName("onClick"));
    EXPECT_TRUE(ast::isValidCallbackName("on_start_game"));
    EXPECT_TRUE(ast::isValidCallbackName("handleClick"));
    EXPECT_FALSE(ast::isValidCallbackName("123invalid"));
    EXPECT_FALSE(ast::isValidCallbackName("on-click"));
    EXPECT_FALSE(ast::isValidCallbackName(""));
}

// ========== AST遍历测试 ==========

class AstTraversalTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建简单的AST用于遍历测试
        m_document = std::make_unique<ast::DocumentNode>();

        auto screen = std::make_unique<ast::ElementNode>(ast::NodeType::Screen);
        screen->tagName = "screen";
        screen->id = "main";

        auto text1 = std::make_unique<ast::ElementNode>(ast::NodeType::Text);
        text1->tagName = "text";
        text1->id = "title";
        screen->children.push_back(std::move(text1));

        auto button = std::make_unique<ast::ElementNode>(ast::NodeType::Button);
        button->tagName = "button";
        button->id = "startBtn";
        screen->children.push_back(std::move(button));

        m_document->children.push_back(std::move(screen));
    }

    std::unique_ptr<ast::DocumentNode> m_document;
};

TEST_F(AstTraversalTest, FindById) {
    auto* found = ast::traversal::findById(*m_document, "title");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->id, "title");

    auto* notFound = ast::traversal::findById(*m_document, "nonexistent");
    EXPECT_EQ(notFound, nullptr);
}

TEST_F(AstTraversalTest, FindByTagName) {
    auto buttons = ast::traversal::findByTagName(*m_document, "button");
    EXPECT_EQ(buttons.size(), 1);

    auto texts = ast::traversal::findByTagName(*m_document, "text");
    EXPECT_EQ(texts.size(), 1);

    auto screens = ast::traversal::findByTagName(*m_document, "screen");
    EXPECT_EQ(screens.size(), 1);
}

TEST_F(AstTraversalTest, CountNodes) {
    auto total = ast::traversal::countNodes(*m_document);
    EXPECT_EQ(total, 4); // document + screen + text + button

    auto textCount = ast::traversal::countNodes(*m_document, ast::NodeType::Text);
    EXPECT_EQ(textCount, 1);

    auto buttonCount = ast::traversal::countNodes(*m_document, ast::NodeType::Button);
    EXPECT_EQ(buttonCount, 1);
}
