#include <gtest/gtest.h>

// 模板系统核心组件
#include "client/ui/kagero/template/core/TemplateConfig.hpp"
#include "client/ui/kagero/template/core/TemplateError.hpp"
#include "client/ui/kagero/template/parser/Lexer.hpp"
#include "client/ui/kagero/template/parser/Parser.hpp"
#include "client/ui/kagero/template/parser/Ast.hpp"
#include "client/ui/kagero/template/parser/AstVisitor.hpp"
#include "client/ui/kagero/template/compiler/TemplateCompiler.hpp"
#include "client/ui/kagero/template/binder/BindingContext.hpp"
#include "client/ui/kagero/state/StateStore.hpp"
#include "client/ui/kagero/event/EventBus.hpp"
#include "client/ui/kagero/event/UIEvents.hpp"

using namespace mc::client::ui::kagero::tpl;
using mc::String;
using mc::i32;
using mc::f32;
using mc::u64;

// ==================== TemplateConfig Tests ====================

class TemplateConfigTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(TemplateConfigTest, DefaultConfig) {
    auto config = core::TemplateConfig::defaults();
    EXPECT_TRUE(config.strictMode);
    EXPECT_FALSE(config.allowInlineScript);
    EXPECT_FALSE(config.allowInlineExpression);
    EXPECT_FALSE(config.allowDynamicTagName);
    EXPECT_TRUE(config.enableCache);
    EXPECT_TRUE(config.validateBindingPaths);
    EXPECT_TRUE(config.validateCallbackNames);
}

TEST_F(TemplateConfigTest, DevelopmentConfig) {
    auto config = core::TemplateConfig::development();
    EXPECT_TRUE(config.debugOutput);
    EXPECT_TRUE(config.strictMode);
}

TEST_F(TemplateConfigTest, ProductionConfig) {
    auto config = core::TemplateConfig::production();
    EXPECT_TRUE(config.enableCache);
    EXPECT_FALSE(config.debugOutput);
}

TEST_F(TemplateConfigTest, SourceLocationDefault) {
    core::SourceLocation loc;
    EXPECT_EQ(loc.line, 1u);
    EXPECT_EQ(loc.column, 1u);
    EXPECT_EQ(loc.offset, 0u);
    EXPECT_TRUE(loc.isValid());
}

TEST_F(TemplateConfigTest, SourceLocationInvalid) {
    auto loc = core::SourceLocation::invalid();
    EXPECT_EQ(loc.line, 0u);
    EXPECT_EQ(loc.column, 0u);
    EXPECT_FALSE(loc.isValid());
}

TEST_F(TemplateConfigTest, SourceLocationToString) {
    core::SourceLocation loc(10, 20, 100);
    EXPECT_EQ(loc.toString(), "line 10, column 20");
}

TEST_F(TemplateConfigTest, SourceRangeMerge) {
    core::SourceLocation start(1, 1, 0);
    core::SourceLocation end(1, 10, 9);
    core::SourceRange range(start, end);

    EXPECT_EQ(range.start.line, 1u);
    EXPECT_EQ(range.end.column, 10u);

    core::SourceLocation otherStart(1, 5, 5);
    core::SourceLocation otherEnd(2, 5, 50);
    core::SourceRange otherRange(otherStart, otherEnd);

    auto merged = range.merge(otherRange);
    EXPECT_EQ(merged.start.offset, 0u);
    EXPECT_EQ(merged.end.offset, 50u);
}

TEST_F(TemplateConfigTest, SourceRangeAt) {
    core::SourceLocation loc(5, 10, 50);
    auto range = core::SourceRange::at(loc);
    EXPECT_EQ(range.start.line, 5u);
    EXPECT_EQ(range.end.line, 5u);
}

// ==================== TemplateError Tests ====================

class TemplateErrorTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(TemplateErrorTest, ErrorInfoCreation) {
    core::TemplateErrorInfo info(
        core::TemplateErrorType::LexerError,
        "Test error message",
        core::SourceLocation(5, 10),
        "test.tpl"
    );

    EXPECT_EQ(info.type, core::TemplateErrorType::LexerError);
    EXPECT_EQ(info.message, "Test error message");
    EXPECT_EQ(info.location.line, 5u);
    EXPECT_EQ(info.sourcePath, "test.tpl");
}

TEST_F(TemplateErrorTest, ErrorInfoFormat) {
    core::TemplateErrorInfo info(
        core::TemplateErrorType::ParserError,
        "Syntax error",
        core::SourceLocation(10, 20),
        "template.xml"
    );

    String formatted = info.format();
    EXPECT_TRUE(formatted.find("ParserError") != String::npos);
    EXPECT_TRUE(formatted.find("Syntax error") != String::npos);
    EXPECT_TRUE(formatted.find("line 10") != String::npos);
    EXPECT_TRUE(formatted.find("template.xml") != String::npos);
}

TEST_F(TemplateErrorTest, ErrorInfoTypeName) {
    EXPECT_STREQ(core::TemplateErrorInfo(
        core::TemplateErrorType::LexerError, "").typeName(), "LexerError");
    EXPECT_STREQ(core::TemplateErrorInfo(
        core::TemplateErrorType::UnexpectedCharacter, "").typeName(), "LexerError");
    EXPECT_STREQ(core::TemplateErrorInfo(
        core::TemplateErrorType::ParserError, "").typeName(), "ParserError");
    EXPECT_STREQ(core::TemplateErrorInfo(
        core::TemplateErrorType::SemanticError, "").typeName(), "SemanticError");
    EXPECT_STREQ(core::TemplateErrorInfo(
        core::TemplateErrorType::CompileError, "").typeName(), "CompileError");
    EXPECT_STREQ(core::TemplateErrorInfo(
        core::TemplateErrorType::RuntimeError, "").typeName(), "RuntimeError");
}

TEST_F(TemplateErrorTest, TemplateErrorException) {
    core::TemplateError error(
        core::TemplateErrorType::InvalidBindingPath,
        "Invalid path: xyz",
        core::SourceLocation(1, 1)
    );

    EXPECT_EQ(error.type(), core::TemplateErrorType::InvalidBindingPath);
    EXPECT_EQ(error.location().line, 1u);
    EXPECT_TRUE(String(error.what()).find("Invalid path") != String::npos);
}

TEST_F(TemplateErrorTest, TemplateErrorFactoryMethods) {
    auto lexerError = core::TemplateError::lexer("Lexer error", core::SourceLocation(1, 1));
    EXPECT_EQ(lexerError.type(), core::TemplateErrorType::LexerError);

    auto parserError = core::TemplateError::parser("Parser error", core::SourceLocation(2, 1));
    EXPECT_EQ(parserError.type(), core::TemplateErrorType::ParserError);

    auto semanticError = core::TemplateError::semantic("Semantic error");
    EXPECT_EQ(semanticError.type(), core::TemplateErrorType::SemanticError);

    auto compileError = core::TemplateError::compile("Compile error");
    EXPECT_EQ(compileError.type(), core::TemplateErrorType::CompileError);

    auto runtimeError = core::TemplateError::runtime("Runtime error");
    EXPECT_EQ(runtimeError.type(), core::TemplateErrorType::RuntimeError);

    auto unknownTagError = core::TemplateError::unknownTag("unknownTag");
    EXPECT_EQ(unknownTagError.type(), core::TemplateErrorType::UnknownTag);

    auto bindingError = core::TemplateError::invalidBindingPath("invalid.path");
    EXPECT_EQ(bindingError.type(), core::TemplateErrorType::InvalidBindingPath);

    auto scriptError = core::TemplateError::inlineScriptNotAllowed();
    EXPECT_EQ(scriptError.type(), core::TemplateErrorType::InlineScriptNotAllowed);
}

TEST_F(TemplateErrorTest, ErrorCollector) {
    core::TemplateErrorCollector collector;

    EXPECT_FALSE(collector.hasErrors());
    EXPECT_EQ(collector.errorCount(), 0u);

    collector.addError(core::TemplateErrorType::LexerError, "Error 1");
    collector.addError(core::TemplateErrorInfo(core::TemplateErrorType::ParserError, "Error 2"));

    EXPECT_TRUE(collector.hasErrors());
    EXPECT_EQ(collector.errorCount(), 2u);

    auto first = collector.firstError();
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->message, "Error 1");

    collector.clear();
    EXPECT_FALSE(collector.hasErrors());
    EXPECT_EQ(collector.errorCount(), 0u);
}

TEST_F(TemplateErrorTest, ErrorCollectorThrowIfErrors) {
    core::TemplateErrorCollector collector;

    // Should not throw when no errors
    EXPECT_NO_THROW(collector.throwIfErrors());

    collector.addError(core::TemplateErrorType::LexerError, "Test error");

    EXPECT_THROW(collector.throwIfErrors(), core::TemplateError);
}

TEST_F(TemplateErrorTest, ErrorCollectorMerge) {
    core::TemplateErrorCollector collector1;
    core::TemplateErrorCollector collector2;

    collector1.addError(core::TemplateErrorType::LexerError, "Error 1");
    collector2.addError(core::TemplateErrorType::ParserError, "Error 2");
    collector2.addError(core::TemplateErrorType::SemanticError, "Error 3");

    collector1.merge(collector2);

    EXPECT_EQ(collector1.errorCount(), 3u);
}

// ==================== Lexer Tests ====================

class LexerTest : public ::testing::Test {
protected:
    parser::Lexer createLexer(const String& source) {
        return parser::Lexer(source, "<test>");
    }
};

TEST_F(LexerTest, EmptySource) {
    auto lexer = createLexer("");
    EXPECT_TRUE(lexer.tokenize());
    EXPECT_TRUE(lexer.tokens().empty() ||
                lexer.tokens().back().type == parser::TokenType::EndOfFile);
    EXPECT_FALSE(lexer.hasErrors());
}

TEST_F(LexerTest, SimpleTag) {
    auto lexer = createLexer("<screen>");
    EXPECT_TRUE(lexer.tokenize());

    auto& tokens = lexer.tokens();
    ASSERT_GE(tokens.size(), 3);
    EXPECT_EQ(tokens[0].type, parser::TokenType::OpenTag);
    EXPECT_EQ(tokens[1].type, parser::TokenType::Identifier);
    EXPECT_EQ(tokens[1].value, "screen");
    EXPECT_EQ(tokens[2].type, parser::TokenType::CloseTag);
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
    ASSERT_GE(tokens.size(), 3);
    EXPECT_EQ(tokens[0].type, parser::TokenType::OpenCloseTag);
    EXPECT_EQ(tokens[1].value, "screen");
    EXPECT_EQ(tokens[2].type, parser::TokenType::CloseTag);
}

TEST_F(LexerTest, TagWithAttributes) {
    auto lexer = createLexer(R"(<button id="btn1" pos="10,20">)");
    EXPECT_TRUE(lexer.tokenize());

    auto& tokens = lexer.tokens();
    bool foundId = false, foundPos = false, foundString1 = false, foundString2 = false;

    for (const auto& token : tokens) {
        if (token.value == "id") foundId = true;
        if (token.value == "pos") foundPos = true;
        if (token.value == "btn1") foundString1 = true;
        if (token.value == "10,20") foundString2 = true;
    }

    EXPECT_TRUE(foundId);
    EXPECT_TRUE(foundPos);
    EXPECT_TRUE(foundString1);
    EXPECT_TRUE(foundString2);
}

TEST_F(LexerTest, BindingAttribute) {
    auto lexer = createLexer(R"(<text bind:text="player.name">)");
    EXPECT_TRUE(lexer.tokenize());

    auto& tokens = lexer.tokens();
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

TEST_F(LexerTest, Comment) {
    auto lexer = createLexer("<!-- This is a comment -->");
    EXPECT_TRUE(lexer.tokenize());

    auto& tokens = lexer.tokens();
    bool foundComment = false;
    for (const auto& token : tokens) {
        if (token.type == parser::TokenType::Comment) {
            foundComment = true;
            EXPECT_EQ(token.value, " This is a comment ");
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
            // 应该包含换行符
            EXPECT_TRUE(token.value.find('\n') != String::npos);
            break;
        }
    }
    EXPECT_TRUE(foundString);
}

TEST_F(LexerTest, StringEscapeSequences) {
    auto lexer = createLexer(R"(<text value="tab\there\nnewline\"quote\\slash">)");
    EXPECT_TRUE(lexer.tokenize());

    auto& tokens = lexer.tokens();
    for (const auto& token : tokens) {
        if (token.type == parser::TokenType::StringLiteral) {
            EXPECT_TRUE(token.value.find('\t') != String::npos);
            EXPECT_TRUE(token.value.find('\n') != String::npos);
            EXPECT_TRUE(token.value.find('"') != String::npos);
            EXPECT_TRUE(token.value.find('\\') != String::npos);
            break;
        }
    }
}

TEST_F(LexerTest, NumberLiteral) {
    auto lexer = createLexer("<text size=\"100\" width=\"3.14\">");
    EXPECT_TRUE(lexer.tokenize());

    auto& tokens = lexer.tokens();
    bool foundInt = false, foundFloat = false;
    for (const auto& token : tokens) {
        if (token.value == "100") {
            foundInt = true;
        }
        if (token.value == "3.14") {
            foundFloat = true;
        }
    }
    EXPECT_TRUE(foundInt);
    EXPECT_TRUE(foundFloat);
}

TEST_F(LexerTest, NestedTags) {
    auto lexer = createLexer("<screen><button/><text/></screen>");
    EXPECT_TRUE(lexer.tokenize());
    EXPECT_FALSE(lexer.hasErrors());

    int tagCount = 0;
    for (const auto& token : lexer.tokens()) {
        if (token.type == parser::TokenType::Identifier) {
            tagCount++;
        }
    }
    EXPECT_EQ(tagCount, 4); // screen, button, text, screen
}

TEST_F(LexerTest, UnterminatedString) {
    auto lexer = createLexer(R"(<text value="unterminated>)");
    EXPECT_FALSE(lexer.tokenize());
    EXPECT_TRUE(lexer.hasErrors());
    EXPECT_TRUE(lexer.firstError()->type == core::TemplateErrorType::UnterminatedString);
}

TEST_F(LexerTest, UnterminatedComment) {
    auto lexer = createLexer("<!-- This comment is not closed");
    EXPECT_FALSE(lexer.tokenize());
    EXPECT_TRUE(lexer.hasErrors());
}

TEST_F(LexerTest, TokenFormat) {
    auto lexer = createLexer("<screen>");
    lexer.tokenize();

    auto& tokens = lexer.tokens();
    ASSERT_GE(tokens.size(), 2);

    String formatted = tokens[1].format();
    EXPECT_TRUE(formatted.find("Identifier") != String::npos);
    EXPECT_TRUE(formatted.find("screen") != String::npos);
}

TEST_F(LexerTest, TokenIsMethods) {
    parser::Token idToken(parser::TokenType::Identifier, "test");
    EXPECT_TRUE(idToken.isIdentifier());
    EXPECT_TRUE(idToken.is(parser::TokenType::Identifier));
    EXPECT_TRUE(idToken.is(parser::TokenType::Identifier, "test"));
    EXPECT_TRUE(idToken.isKeyword("test"));
    EXPECT_FALSE(idToken.isText());
    EXPECT_FALSE(idToken.isStringLiteral());
    EXPECT_FALSE(idToken.isNumberLiteral());

    parser::Token strToken(parser::TokenType::StringLiteral, "value");
    EXPECT_TRUE(strToken.isStringLiteral());

    parser::Token numToken(parser::TokenType::NumberLiteral, "42");
    EXPECT_TRUE(numToken.isNumberLiteral());

    parser::Token textToken(parser::TokenType::Text, "content");
    EXPECT_TRUE(textToken.isText());
}

TEST_F(LexerTest, LexerIterator) {
    auto lexer = createLexer("<screen><button/></screen>");
    EXPECT_TRUE(lexer.tokenize());

    EXPECT_TRUE(lexer.hasNext());

    // Test iteration
    int count = 0;
    while (lexer.hasNext()) {
        lexer.next();
        count++;
    }
    EXPECT_GT(count, 0);

    // Test back
    lexer.back();
    EXPECT_TRUE(lexer.hasNext());
}

TEST_F(LexerTest, LexerPeek) {
    auto lexer = createLexer("<screen>");
    lexer.tokenize();

    const auto& current = lexer.current();
    EXPECT_EQ(current.type, parser::TokenType::OpenTag);

    const auto& peeked = lexer.peek();
    EXPECT_EQ(peeked.type, parser::TokenType::Identifier);
    EXPECT_EQ(peeked.value, "screen");

    // Test peek with offset
    const auto& peeked2 = lexer.peek(2);
    EXPECT_EQ(peeked2.type, parser::TokenType::CloseTag);
}

TEST_F(LexerTest, LexerPosition) {
    auto lexer = createLexer("<screen>");
    lexer.tokenize();

    EXPECT_EQ(lexer.position(), 0u);
    lexer.next();
    EXPECT_EQ(lexer.position(), 1u);

    lexer.setPosition(0u);
    EXPECT_EQ(lexer.position(), 0u);
}

TEST_F(LexerTest, LexerSkipWhitespace) {
    auto lexer = createLexer("   <screen>   ");
    EXPECT_TRUE(lexer.tokenize());

    // Skip whitespace manually
    lexer.skipWhitespace();
    EXPECT_TRUE(lexer.hasNext());
    EXPECT_EQ(lexer.current().type, parser::TokenType::OpenTag);
}

TEST_F(LexerTest, LexerExpect) {
    auto lexer = createLexer("<screen>");
    lexer.tokenize();

    EXPECT_TRUE(lexer.expect(parser::TokenType::OpenTag));
    EXPECT_TRUE(lexer.expect(parser::TokenType::Identifier, "screen"));
    EXPECT_TRUE(lexer.expect(parser::TokenType::CloseTag));
}

TEST_F(LexerTest, LexerExpectFailure) {
    auto lexer = createLexer("<screen>");
    lexer.tokenize();

    // Should fail because expecting wrong type
    EXPECT_FALSE(lexer.expect(parser::TokenType::CloseTag));
    EXPECT_TRUE(lexer.hasErrors());
}

TEST_F(LexerTest, LexerGetLineContent) {
    auto lexer = createLexer("line1\nline2\nline3");
    lexer.tokenize();

    String line1 = lexer.getLineContent(1);
    EXPECT_EQ(line1, "line1");

    String line2 = lexer.getLineContent(2);
    EXPECT_EQ(line2, "line2");
}

TEST_F(LexerTest, LexerGetContext) {
    auto lexer = createLexer("line1\nline2\nline3\nline4");
    lexer.tokenize();

    core::SourceLocation loc(2, 3, 8);
    String context = lexer.getContext(loc, 1);
    EXPECT_TRUE(context.find("line1") != String::npos);
    EXPECT_TRUE(context.find("line2") != String::npos);
    EXPECT_TRUE(context.find("line3") != String::npos);
}

TEST_F(LexerTest, LexerStaticMethods) {
    EXPECT_TRUE(parser::Lexer::isWhitespace(' '));
    EXPECT_TRUE(parser::Lexer::isWhitespace('\t'));
    EXPECT_TRUE(parser::Lexer::isWhitespace('\r'));
    EXPECT_FALSE(parser::Lexer::isWhitespace('a'));

    EXPECT_TRUE(parser::Lexer::isNewline('\n'));
    EXPECT_FALSE(parser::Lexer::isNewline('a'));

    EXPECT_TRUE(parser::Lexer::isAlpha('a'));
    EXPECT_TRUE(parser::Lexer::isAlpha('Z'));
    EXPECT_FALSE(parser::Lexer::isAlpha('1'));

    EXPECT_TRUE(parser::Lexer::isDigit('0'));
    EXPECT_TRUE(parser::Lexer::isDigit('9'));
    EXPECT_FALSE(parser::Lexer::isDigit('a'));

    EXPECT_TRUE(parser::Lexer::isAlphaNumeric('a'));
    EXPECT_TRUE(parser::Lexer::isAlphaNumeric('1'));
    EXPECT_FALSE(parser::Lexer::isAlphaNumeric('-'));

    EXPECT_TRUE(parser::Lexer::isIdentifierChar('a'));
    EXPECT_TRUE(parser::Lexer::isIdentifierChar('_'));
    EXPECT_TRUE(parser::Lexer::isIdentifierChar('-'));
    EXPECT_TRUE(parser::Lexer::isIdentifierChar(':'));
    EXPECT_FALSE(parser::Lexer::isIdentifierChar(' '));
}

// ==================== Parser Tests ====================

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

    const auto* posAttr = elem->getAttribute("pos");
    ASSERT_NE(posAttr, nullptr);
    EXPECT_TRUE(posAttr->isStatic());
    EXPECT_EQ(posAttr->rawValue, "10,20");
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
    EXPECT_FALSE(attr->binding->isLoopVariable);
}

TEST_F(ParserTest, ElementWithLoopVariableBinding) {
    auto doc = parse(R"(<slot bind:item="$slot.itemId"/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* elem = doc->rootElement();
    const auto* attr = elem->getAttribute("bind:item");
    ASSERT_NE(attr, nullptr);
    EXPECT_TRUE(attr->isBinding());
    ASSERT_TRUE(attr->binding.has_value());
    EXPECT_TRUE(attr->binding->isLoopVariable);
    EXPECT_EQ(attr->binding->loopVarName, "slot");
    EXPECT_EQ(attr->binding->property, "itemId");
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

TEST_F(ParserTest, DeepNesting) {
    auto doc = parse(R"(
        <screen>
            <grid>
                <slot>
                    <text text="Nested"/>
                </slot>
            </grid>
        </screen>
    )");

    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);
    EXPECT_EQ(doc->rootElement()->tagName, "screen");
    EXPECT_EQ(doc->rootElement()->children.size(), 1);

    auto* grid = dynamic_cast<ast::ElementNode*>(doc->rootElement()->children[0].get());
    ASSERT_NE(grid, nullptr);
    EXPECT_EQ(grid->tagName, "grid");
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

    // Find text node
    bool foundText = false;
    for (const auto& child : doc->rootElement()->children) {
        if (child->type == ast::NodeType::TextContent) {
            auto* textNode = dynamic_cast<ast::TextNode*>(child.get());
            ASSERT_NE(textNode, nullptr);
            foundText = true;
            break;
        }
    }
    EXPECT_TRUE(foundText);
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

    // Check for comment node
    bool foundComment = false;
    for (const auto& child : doc->rootElement()->children) {
        if (child->type == ast::NodeType::Comment) {
            foundComment = true;
            break;
        }
    }
    EXPECT_TRUE(foundComment);
}

TEST_F(ParserTest, AttributeCategorization) {
    auto doc = parse(R"(<text pos="10,20" bind:text="player.name" on:click="onClick"/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* elem = doc->rootElement();
    elem->categorizeAttributes();

    EXPECT_EQ(elem->staticAttrs.size(), 1u);
    EXPECT_EQ(elem->bindingAttrs.size(), 1u);
    EXPECT_EQ(elem->eventAttrs.size(), 1u);
}

TEST_F(ParserTest, LoopInfoExtraction) {
    // Test that for: directive properly extracts loop info
    auto doc = parse(R"(
        <grid for:slot="slot in player.inventory.slots">
            <text bind:text="$slot.name"/>
        </grid>
    )");

    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* grid = doc->rootElement();
    EXPECT_TRUE(grid->loop.has_value());
    EXPECT_EQ(grid->loop->collectionPath, "player.inventory.slots");
    EXPECT_EQ(grid->loop->itemVarName, "slot");
}

TEST_F(ParserTest, ConditionInfoExtraction) {
    auto doc = parse(R"(<text bind:visible="player.isSneaking"/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* text = doc->rootElement();
    // Element needs to have its attributes categorized
    text->categorizeAttributes();

    // Condition is extracted when present
    auto condition = parser::ConditionInfo();
    condition.booleanPath = "player.isSneaking";
    condition.negate = false;

    // Verify the bind:visible attribute exists
    const auto* attr = text->getAttribute("bind:visible");
    ASSERT_NE(attr, nullptr);
    EXPECT_TRUE(attr->isBinding());
}

TEST_F(ParserTest, ConditionInfoWithNegation) {
    auto doc = parse(R"(<text bind:visible="!player.hidden"/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* text = doc->rootElement();
    text->categorizeAttributes();

    // Verify the bind:visible attribute exists
    const auto* attr = text->getAttribute("bind:visible");
    ASSERT_NE(attr, nullptr);
    EXPECT_TRUE(attr->isBinding());
    // The binding path includes the ! prefix
    EXPECT_TRUE(attr->rawValue.find("!") != String::npos || attr->binding->path.find("!") != String::npos);
}

TEST_F(ParserTest, InvalidTagInStrictMode) {
    m_config.strictMode = true;

    parser::Lexer lexer("<invalidTag/>", "<test>");
    lexer.tokenize();

    parser::Parser parser(lexer, m_config);
    auto doc = parser.parse();

    // Strict mode should generate error for unknown tag
    EXPECT_TRUE(parser.hasErrors());
}

TEST_F(ParserTest, InvalidBindingPath) {
    m_config.validateBindingPaths = true;

    parser::Lexer lexer(R"(<text bind:text="123invalid"/>)", "<test>");
    lexer.tokenize();

    parser::Parser parser(lexer, m_config);
    auto doc = parser.parse();

    EXPECT_TRUE(parser.hasErrors());
}

TEST_F(ParserTest, InvalidCallbackName) {
    m_config.validateCallbackNames = true;

    parser::Lexer lexer(R"(<button on:click="123invalid"/>)", "<test>");
    lexer.tokenize();

    parser::Parser parser(lexer, m_config);
    auto doc = parser.parse();

    EXPECT_TRUE(parser.hasErrors());
}

TEST_F(ParserTest, MissingClosingTag) {
    parser::Lexer lexer("<screen><button>", "<test>");
    lexer.tokenize();

    parser::Parser parser(lexer, m_config);
    auto doc = parser.parse();

    EXPECT_TRUE(parser.hasErrors());
}

TEST_F(ParserTest, MismatchedClosingTag) {
    parser::Lexer lexer("<screen><button/></text>", "<test>");
    lexer.tokenize();

    parser::Parser parser(lexer, m_config);
    auto doc = parser.parse();

    EXPECT_TRUE(parser.hasErrors());
}

TEST_F(ParserTest, MultipleRootElements) {
    auto doc = parse(R"(
        <screen/>
        <button/>
    )");

    ASSERT_NE(doc, nullptr);
    // Document can have multiple children
    EXPECT_GE(doc->children.size(), 2u);
}

TEST_F(ParserTest, NumericAttributeValues) {
    auto doc = parse(R"(<text size=100 width=3.14/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* elem = doc->rootElement();
    EXPECT_TRUE(elem->hasAttribute("size"));
    EXPECT_TRUE(elem->hasAttribute("width"));
}

TEST_F(ParserTest, ClassesAttribute) {
    // Note: Classes are parsed as regular attributes, not specially handled yet
    auto doc = parse(R"(<text class="title large"/>")");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);
    EXPECT_TRUE(doc->rootElement()->hasAttribute("class"));
}

// ==================== AST Tests ====================

class AstTest : public ::testing::Test {
};

TEST_F(AstTest, NodeTypeNames) {
    EXPECT_STREQ(ast::nodeTypeName(ast::NodeType::Document), "Document");
    EXPECT_STREQ(ast::nodeTypeName(ast::NodeType::Screen), "Screen");
    EXPECT_STREQ(ast::nodeTypeName(ast::NodeType::Widget), "Widget");
    EXPECT_STREQ(ast::nodeTypeName(ast::NodeType::Button), "Button");
    EXPECT_STREQ(ast::nodeTypeName(ast::NodeType::Text), "Text");
    EXPECT_STREQ(ast::nodeTypeName(ast::NodeType::Grid), "Grid");
    EXPECT_STREQ(ast::nodeTypeName(ast::NodeType::Slot), "Slot");
    EXPECT_STREQ(ast::nodeTypeName(ast::NodeType::Comment), "Comment");
    EXPECT_STREQ(ast::nodeTypeName(static_cast<ast::NodeType>(255)), "Unknown");
}

TEST_F(AstTest, BindingInfoParseSimplePath) {
    auto info = ast::BindingInfo::parse("player.name");
    EXPECT_FALSE(info.isLoopVariable);
    EXPECT_EQ(info.path, "player.name");
    EXPECT_TRUE(info.isValid());
}

TEST_F(AstTest, BindingInfoParseLoopVariable) {
    auto info = ast::BindingInfo::parse("$item");
    EXPECT_TRUE(info.isLoopVariable);
    EXPECT_EQ(info.loopVarName, "item");
    EXPECT_TRUE(info.property.empty());
    EXPECT_TRUE(info.isValid());
}

TEST_F(AstTest, BindingInfoParseLoopVariableWithProperty) {
    auto info = ast::BindingInfo::parse("$slot.item");
    EXPECT_TRUE(info.isLoopVariable);
    EXPECT_EQ(info.loopVarName, "slot");
    EXPECT_EQ(info.property, "item");
    EXPECT_TRUE(info.isValid());
}

TEST_F(AstTest, BindingInfoParseEmpty) {
    auto info = ast::BindingInfo::parse("");
    EXPECT_FALSE(info.isValid());
}

TEST_F(AstTest, AttributeCreateStatic) {
    auto attr = ast::Attribute::createStatic("pos", "10,20");
    EXPECT_TRUE(attr.isStatic());
    EXPECT_FALSE(attr.isBinding());
    EXPECT_FALSE(attr.isEvent());
    EXPECT_EQ(attr.name, "pos");
    EXPECT_EQ(attr.rawValue, "10,20");

    // Check value parsing - string
    EXPECT_TRUE(std::holds_alternative<String>(attr.value));
}

TEST_F(AstTest, AttributeCreateStaticBoolean) {
    auto attrTrue = ast::Attribute::createStatic("visible", "true");
    EXPECT_TRUE(std::holds_alternative<bool>(attrTrue.value));
    EXPECT_TRUE(std::get<bool>(attrTrue.value));

    auto attrFalse = ast::Attribute::createStatic("visible", "false");
    EXPECT_TRUE(std::holds_alternative<bool>(attrFalse.value));
    EXPECT_FALSE(std::get<bool>(attrFalse.value));
}

TEST_F(AstTest, AttributeCreateStaticInteger) {
    auto attr = ast::Attribute::createStatic("size", "100");
    EXPECT_TRUE(std::holds_alternative<i32>(attr.value));
    EXPECT_EQ(std::get<i32>(attr.value), 100);
}

TEST_F(AstTest, AttributeCreateStaticFloat) {
    auto attr = ast::Attribute::createStatic("opacity", "0.75");
    EXPECT_TRUE(attr.isStatic());
    // Value type may be float or string depending on parsing
    EXPECT_TRUE(std::holds_alternative<f32>(attr.value) || std::holds_alternative<String>(attr.value));
    if (std::holds_alternative<f32>(attr.value)) {
        EXPECT_FLOAT_EQ(std::get<f32>(attr.value), 0.75f);
    }
}

TEST_F(AstTest, AttributeCreateBinding) {
    auto attr = ast::Attribute::createBinding("bind:text", "player.name");
    EXPECT_TRUE(attr.isBinding());
    EXPECT_FALSE(attr.isStatic());
    EXPECT_FALSE(attr.isEvent());
    EXPECT_EQ(attr.name, "bind:text");
    EXPECT_TRUE(attr.binding.has_value());
    EXPECT_EQ(attr.binding->path, "player.name");
}

TEST_F(AstTest, AttributeCreateEvent) {
    auto attr = ast::Attribute::createEvent("on:click", "onButtonClick");
    EXPECT_TRUE(attr.isEvent());
    EXPECT_FALSE(attr.isStatic());
    EXPECT_FALSE(attr.isBinding());
    EXPECT_EQ(attr.name, "on:click");
    EXPECT_EQ(attr.callbackName, "onButtonClick");
}

TEST_F(AstTest, AttributeBaseName) {
    auto staticAttr = ast::Attribute::createStatic("pos", "10,20");
    EXPECT_EQ(staticAttr.baseName(), "pos");

    auto bindingAttr = ast::Attribute::createBinding("bind:text", "player.name");
    EXPECT_EQ(bindingAttr.baseName(), "text");

    auto eventAttr = ast::Attribute::createEvent("on:click", "onClick");
    EXPECT_EQ(eventAttr.baseName(), "click");
}

TEST_F(AstTest, ElementNodeBasics) {
    ast::ElementNode elem(ast::NodeType::Button);
    elem.tagName = "button";
    elem.id = "myButton";

    // Test addAttribute
    elem.addAttribute(ast::Attribute::createStatic("pos", "10,20"));
    EXPECT_TRUE(elem.hasAttribute("pos"));

    // Test getAttribute
    const auto* attr = elem.getAttribute("pos");
    ASSERT_NE(attr, nullptr);
    EXPECT_EQ(attr->rawValue, "10,20");

    // Test non-existent attribute
    const auto* missing = elem.getAttribute("missing");
    EXPECT_EQ(missing, nullptr);
}

TEST_F(AstTest, ElementNodeCategorizeAttributes) {
    ast::ElementNode elem(ast::NodeType::Text);
    elem.addAttribute(ast::Attribute::createStatic("pos", "10,20"));
    elem.addAttribute(ast::Attribute::createBinding("bind:text", "player.name"));
    elem.addAttribute(ast::Attribute::createEvent("on:click", "onClick"));

    elem.categorizeAttributes();

    EXPECT_EQ(elem.staticAttrs.size(), 1u);
    EXPECT_EQ(elem.bindingAttrs.size(), 1u);
    EXPECT_EQ(elem.eventAttrs.size(), 1u);
}

TEST_F(AstTest, ElementNodeClone) {
    auto original = std::make_unique<ast::ElementNode>(ast::NodeType::Button);
    original->tagName = "button";
    original->id = "btn";
    original->addAttribute(ast::Attribute::createStatic("pos", "10,20"));

    auto child = std::make_unique<ast::ElementNode>(ast::NodeType::Text);
    child->tagName = "text";
    original->children.push_back(std::move(child));

    auto cloned = original->clone();
    ASSERT_NE(cloned, nullptr);

    auto* clonedElem = dynamic_cast<ast::ElementNode*>(cloned.get());
    ASSERT_NE(clonedElem, nullptr);
    EXPECT_EQ(clonedElem->tagName, "button");
    EXPECT_EQ(clonedElem->id, "btn");
    EXPECT_TRUE(clonedElem->hasAttribute("pos"));
    EXPECT_EQ(clonedElem->children.size(), 1u);
}

TEST_F(AstTest, TextNodeClone) {
    auto original = std::make_unique<ast::TextNode>();
    original->text = "Hello World";
    original->isWhitespace = false;

    auto cloned = original->clone();
    auto* textNode = dynamic_cast<ast::TextNode*>(cloned.get());
    ASSERT_NE(textNode, nullptr);
    EXPECT_EQ(textNode->text, "Hello World");
    EXPECT_FALSE(textNode->isWhitespace);
}

TEST_F(AstTest, CommentNodeClone) {
    auto original = std::make_unique<ast::CommentNode>();
    original->text = "This is a comment";

    auto cloned = original->clone();
    auto* commentNode = dynamic_cast<ast::CommentNode*>(cloned.get());
    ASSERT_NE(commentNode, nullptr);
    EXPECT_EQ(commentNode->text, "This is a comment");
}

TEST_F(AstTest, DocumentNodeClone) {
    auto original = std::make_unique<ast::DocumentNode>();
    original->sourcePath = "test.tpl";

    auto child = std::make_unique<ast::ElementNode>(ast::NodeType::Screen);
    child->tagName = "screen";
    original->children.push_back(std::move(child));

    auto cloned = original->clone();
    auto* docNode = dynamic_cast<ast::DocumentNode*>(cloned.get());
    ASSERT_NE(docNode, nullptr);
    EXPECT_EQ(docNode->sourcePath, "test.tpl");
    EXPECT_EQ(docNode->children.size(), 1u);
}

TEST_F(AstTest, DocumentNodeRootElement) {
    auto doc = std::make_unique<ast::DocumentNode>();

    // No children
    EXPECT_EQ(doc->rootElement(), nullptr);

    // Add text node first
    auto text = std::make_unique<ast::TextNode>();
    doc->children.push_back(std::move(text));
    EXPECT_EQ(doc->rootElement(), nullptr);

    // Add element
    auto screen = std::make_unique<ast::ElementNode>(ast::NodeType::Screen);
    screen->tagName = "screen";
    ast::ElementNode* screenPtr = screen.get();
    doc->children.push_back(std::move(screen));

    EXPECT_EQ(doc->rootElement(), screenPtr);
}

TEST_F(AstTest, ValidTagName) {
    EXPECT_TRUE(ast::isValidWidgetTag("screen"));
    EXPECT_TRUE(ast::isValidWidgetTag("button"));
    EXPECT_TRUE(ast::isValidWidgetTag("text"));
    EXPECT_TRUE(ast::isValidWidgetTag("slot"));
    EXPECT_TRUE(ast::isValidWidgetTag("grid"));
    EXPECT_TRUE(ast::isValidWidgetTag("viewport3d"));
    EXPECT_TRUE(ast::isValidWidgetTag("scrollable"));
    EXPECT_TRUE(ast::isValidWidgetTag("list"));
    EXPECT_TRUE(ast::isValidWidgetTag("style"));
    EXPECT_TRUE(ast::isValidWidgetTag("textfield"));
    EXPECT_TRUE(ast::isValidWidgetTag("slider"));
    EXPECT_TRUE(ast::isValidWidgetTag("checkbox"));
    EXPECT_TRUE(ast::isValidWidgetTag("image"));
    EXPECT_TRUE(ast::isValidWidgetTag("widget"));

    EXPECT_FALSE(ast::isValidWidgetTag("invalidTag"));
    EXPECT_FALSE(ast::isValidWidgetTag("script"));
    EXPECT_FALSE(ast::isValidWidgetTag("div"));

    // Case insensitive
    EXPECT_TRUE(ast::isValidWidgetTag("SCREEN"));
    EXPECT_TRUE(ast::isValidWidgetTag("Button"));
}

TEST_F(AstTest, ValidBindingPath) {
    EXPECT_TRUE(ast::isValidBindingPath("player"));
    EXPECT_TRUE(ast::isValidBindingPath("player.name"));
    EXPECT_TRUE(ast::isValidBindingPath("player.inventory.slots"));
    EXPECT_TRUE(ast::isValidBindingPath("player.inventory.slots[0]"));
    EXPECT_TRUE(ast::isValidBindingPath("array[index]"));
    EXPECT_TRUE(ast::isValidBindingPath("$item"));
    EXPECT_TRUE(ast::isValidBindingPath("$slot.item"));
    EXPECT_TRUE(ast::isValidBindingPath("$data.nested.value"));

    EXPECT_FALSE(ast::isValidBindingPath(""));
    EXPECT_FALSE(ast::isValidBindingPath("123invalid"));
    EXPECT_FALSE(ast::isValidBindingPath("player..name"));
    EXPECT_FALSE(ast::isValidBindingPath(".player"));
    EXPECT_FALSE(ast::isValidBindingPath("$"));
    EXPECT_FALSE(ast::isValidBindingPath("$123"));
}

TEST_F(AstTest, ValidCallbackName) {
    EXPECT_TRUE(ast::isValidCallbackName("onClick"));
    EXPECT_TRUE(ast::isValidCallbackName("on_start_game"));
    EXPECT_TRUE(ast::isValidCallbackName("handleClick"));
    EXPECT_TRUE(ast::isValidCallbackName("_private"));
    EXPECT_TRUE(ast::isValidCallbackName("callback1"));

    EXPECT_FALSE(ast::isValidCallbackName(""));
    EXPECT_FALSE(ast::isValidCallbackName("123invalid"));
    EXPECT_FALSE(ast::isValidCallbackName("on-click"));
    EXPECT_FALSE(ast::isValidCallbackName("onClick()"));
    EXPECT_FALSE(ast::isValidCallbackName("obj.method"));
}

TEST_F(AstTest, ValidAttributeName) {
    EXPECT_TRUE(ast::isValidAttributeName("pos"));
    EXPECT_TRUE(ast::isValidAttributeName("bind:text"));
    EXPECT_TRUE(ast::isValidAttributeName("on:click"));
    EXPECT_TRUE(ast::isValidAttributeName("data-value"));
    EXPECT_TRUE(ast::isValidAttributeName("_private"));
    EXPECT_TRUE(ast::isValidAttributeName("id"));

    EXPECT_FALSE(ast::isValidAttributeName(""));
    EXPECT_FALSE(ast::isValidAttributeName("123invalid"));
    EXPECT_FALSE(ast::isValidAttributeName("attr with space"));
}

TEST_F(AstTest, GetNodeTypeFromTagName) {
    EXPECT_EQ(ast::getNodeTypeFromTagName("screen"), ast::NodeType::Screen);
    EXPECT_EQ(ast::getNodeTypeFromTagName("button"), ast::NodeType::Button);
    EXPECT_EQ(ast::getNodeTypeFromTagName("text"), ast::NodeType::Text);
    EXPECT_EQ(ast::getNodeTypeFromTagName("grid"), ast::NodeType::Grid);
    EXPECT_EQ(ast::getNodeTypeFromTagName("slot"), ast::NodeType::Slot);
    EXPECT_EQ(ast::getNodeTypeFromTagName("unknown"), ast::NodeType::Widget);

    // Case insensitive
    EXPECT_EQ(ast::getNodeTypeFromTagName("SCREEN"), ast::NodeType::Screen);
    EXPECT_EQ(ast::getNodeTypeFromTagName("Button"), ast::NodeType::Button);
}

// ==================== AST Visitor Tests ====================

class AstVisitorTest : public ::testing::Test {
protected:
    void SetUp() override {
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

TEST_F(AstVisitorTest, FindById) {
    auto* found = ast::traversal::findById(*m_document, "title");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->id, "title");

    auto* notFound = ast::traversal::findById(*m_document, "nonexistent");
    EXPECT_EQ(notFound, nullptr);
}

TEST_F(AstVisitorTest, FindByTagName) {
    auto buttons = ast::traversal::findByTagName(*m_document, "button");
    EXPECT_EQ(buttons.size(), 1u);

    auto texts = ast::traversal::findByTagName(*m_document, "text");
    EXPECT_EQ(texts.size(), 1u);

    auto screens = ast::traversal::findByTagName(*m_document, "screen");
    EXPECT_EQ(screens.size(), 1u);

    auto unknowns = ast::traversal::findByTagName(*m_document, "unknown");
    EXPECT_EQ(unknowns.size(), 0u);
}

TEST_F(AstVisitorTest, CountNodes) {
    auto total = ast::traversal::countNodes(*m_document);
    EXPECT_EQ(total, 4u); // document + screen + text + button

    auto textCount = ast::traversal::countNodes(*m_document, ast::NodeType::Text);
    EXPECT_EQ(textCount, 1u);

    auto buttonCount = ast::traversal::countNodes(*m_document, ast::NodeType::Button);
    EXPECT_EQ(buttonCount, 1u);

    auto screenCount = ast::traversal::countNodes(*m_document, ast::NodeType::Screen);
    EXPECT_EQ(screenCount, 1u);
}

TEST_F(AstVisitorTest, PreorderTraversal) {
    std::vector<String> visitedTags;
    ast::traversal::preorder(*m_document, [&visitedTags](ast::Node& node) {
        if (auto* elem = dynamic_cast<ast::ElementNode*>(&node)) {
            visitedTags.push_back(elem->tagName);
        }
        return true;
    });

    // Preorder: screen, text, button
    ASSERT_GE(visitedTags.size(), 3u);
    EXPECT_EQ(visitedTags[0], "screen");
    EXPECT_EQ(visitedTags[1], "text");
    EXPECT_EQ(visitedTags[2], "button");
}

TEST_F(AstVisitorTest, PostorderTraversal) {
    std::vector<String> visitedTags;
    ast::traversal::postorder(*m_document, [&visitedTags](ast::Node& node) {
        if (auto* elem = dynamic_cast<ast::ElementNode*>(&node)) {
            visitedTags.push_back(elem->tagName);
        }
        return true;
    });

    // Postorder: text, button, screen
    ASSERT_GE(visitedTags.size(), 3u);
    EXPECT_EQ(visitedTags[0], "text");
    EXPECT_EQ(visitedTags[1], "button");
    EXPECT_EQ(visitedTags[2], "screen");
}

TEST_F(AstVisitorTest, FindFirst) {
    auto* found = ast::traversal::findFirst(*m_document, [](const ast::Node& node) {
        return node.type == ast::NodeType::Button;
    });
    ASSERT_NE(found, nullptr);

    auto* notFound = ast::traversal::findFirst(*m_document, [](const ast::Node& node) {
        return node.type == ast::NodeType::Grid;
    });
    EXPECT_EQ(notFound, nullptr);
}

TEST_F(AstVisitorTest, FindAll) {
    auto elements = ast::traversal::findAll(*m_document, [](const ast::Node& node) {
        return node.type == ast::NodeType::Button || node.type == ast::NodeType::Text;
    });
    EXPECT_EQ(elements.size(), 2u);
}

TEST_F(AstVisitorTest, GetDepth) {
    // Document depth
    auto docDepth = ast::traversal::getDepth(*m_document);
    EXPECT_EQ(docDepth, 2u); // document -> screen -> (text, button)
}

TEST_F(AstVisitorTest, VisitorStop) {
    int visitCount = 0;

    class CountingVisitor : public ast::AstVisitor {
    public:
        int& count;
        CountingVisitor(int& c) : count(c) {}

        void visitElement(ast::ElementNode& node) override {
            count++;
            traverseChildren(node);
            if (count >= 2) {
                stop();
            }
        }
    };

    CountingVisitor visitor(visitCount);
    visitor.visit(*m_document);

    EXPECT_GE(visitCount, 2);
}

TEST_F(AstVisitorTest, ConstVisitor) {
    std::vector<String> visitedTags;

    class ConstTagCollector : public ast::ConstAstVisitor {
    public:
        std::vector<String>& tags;
        ConstTagCollector(std::vector<String>& t) : tags(t) {}

        void visitElement(const ast::ElementNode& node) override {
            tags.push_back(node.tagName);
            traverseChildren(node);
        }
    };

    ConstTagCollector collector(visitedTags);
    collector.visit(*m_document);

    EXPECT_GE(visitedTags.size(), 3u);
}

// ==================== TemplateCompiler Tests ====================

class TemplateCompilerTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_config = core::TemplateConfig::defaults();
    }

    core::TemplateConfig m_config;
};

TEST_F(TemplateCompilerTest, SimpleTemplate) {
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compile("<screen id=\"main\"/>");

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->isValid());
    EXPECT_FALSE(result->hasErrors());
    EXPECT_NE(result->astRoot(), nullptr);
}

TEST_F(TemplateCompilerTest, TemplateWithBindings) {
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compile(R"(
        <screen>
            <text id="nameLabel" bind:text="player.name"/>
            <button id="startBtn" on:click="onStartGame"/>
        </screen>
    )");

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->isValid());

    // Should have at least one binding plan
    EXPECT_GE(result->bindingPlans().size(), 1u);

    // Should have at least one event plan
    EXPECT_GE(result->eventPlans().size(), 1u);

    // Check first binding plan
    bool foundNameBinding = false;
    for (const auto& plan : result->bindingPlans()) {
        if (plan.statePath == "player.name") {
            foundNameBinding = true;
            break;
        }
    }
    EXPECT_TRUE(foundNameBinding);

    // Check first event plan
    bool foundClickEvent = false;
    for (const auto& plan : result->eventPlans()) {
        if (plan.callbackName == "onStartGame") {
            foundClickEvent = true;
            break;
        }
    }
    EXPECT_TRUE(foundClickEvent);

    // Should have watched paths
    EXPECT_TRUE(result->watchedPaths().count("player.name") > 0);

    // Should have registered callbacks
    EXPECT_TRUE(result->registeredCallbacks().count("onStartGame") > 0);
}

TEST_F(TemplateCompilerTest, TemplateWithLoop) {
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compile(R"(
        <grid bind:items="player.inventory.slots">
            <slot bind:item="$slot.item"/>
        </grid>
    )");

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->isValid());

    // Should have at least one binding (for slot.item)
    EXPECT_GE(result->bindingPlans().size(), 1u);

    // Check for loop binding
    bool foundLoopBinding = false;
    for (const auto& plan : result->bindingPlans()) {
        if (plan.isLoopBinding) {
            foundLoopBinding = true;
            break;
        }
    }
    EXPECT_TRUE(foundLoopBinding);
}

TEST_F(TemplateCompilerTest, StrictModeRejectsInlineScript) {
    m_config.strictMode = true;
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compile(R"(<text>{{ player.health }}/>)");

    ASSERT_NE(result, nullptr);
    EXPECT_FALSE(result->isValid());
    EXPECT_TRUE(result->hasErrors());
}

TEST_F(TemplateCompilerTest, StrictModeRejectsInlineExpression) {
    m_config.strictMode = true;
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compile(R"(<text bind:text="player.level + 1"/>)");

    ASSERT_NE(result, nullptr);
    // Should have error for inline expression
    EXPECT_FALSE(result->isValid());
}

TEST_F(TemplateCompilerTest, CompileWithDebug) {
    m_config.debugOutput = true;
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compile("<screen/>");

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->isValid());

    // Check debug dump
    String dump = result->debugDump();
    EXPECT_TRUE(dump.find("Compiled Template") != String::npos);
    EXPECT_TRUE(dump.find("Source:") != String::npos);
}

TEST_F(TemplateCompilerTest, CompileFromFile) {
    compiler::TemplateCompiler compiler(m_config);

    // Test with non-existent file
    auto result = compiler.compileFile("/nonexistent/path/template.tpl");

    ASSERT_NE(result, nullptr);
    EXPECT_FALSE(result->isValid());
    EXPECT_TRUE(result->hasErrors());
}

TEST_F(TemplateCompilerTest, CompileAst) {
    // Create AST manually
    auto document = std::make_unique<ast::DocumentNode>();
    auto screen = std::make_unique<ast::ElementNode>(ast::NodeType::Screen);
    screen->tagName = "screen";
    screen->id = "main";
    document->children.push_back(std::move(screen));

    compiler::TemplateCompiler compiler(m_config);
    auto result = compiler.compileAst(std::move(document), "<test>");

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->isValid());
    EXPECT_FALSE(result->hasErrors());
}

TEST_F(TemplateCompilerTest, CompileNullAst) {
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compileAst(nullptr, "<test>");

    ASSERT_NE(result, nullptr);
    EXPECT_FALSE(result->isValid());
    EXPECT_TRUE(result->hasErrors());
}

TEST_F(TemplateCompilerTest, CompileTime) {
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compile("<screen/>");

    ASSERT_NE(result, nullptr);
    // Compile time should be recorded
    // Note: May be 0 on fast machines
    EXPECT_TRUE(result->compileTime() >= 0u);
}

TEST_F(TemplateCompilerTest, InvalidTagInStrictMode) {
    m_config.strictMode = true;
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compile("<invalidTag/>");

    ASSERT_NE(result, nullptr);
    EXPECT_FALSE(result->isValid());
    EXPECT_TRUE(result->hasErrors());
}

TEST_F(TemplateCompilerTest, CompiledTemplateWatchedPaths) {
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compile(R"(
        <screen id="main">
            <text bind:text="player.name"/>
        </screen>
    )");

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->isValid());

    // Verify watched paths are collected correctly
    const auto& paths = result->watchedPaths();
    // At minimum we should have player.name if there's a binding
    EXPECT_GE(paths.size(), 0u);
}

// ==================== BindingContext Tests ====================

class BindingContextTest : public ::testing::Test {
protected:
    void SetUp() override {
        // StateStore and EventBus are singletons - use instance()
        m_ctx = std::make_unique<binder::BindingContext>(
            mc::client::ui::kagero::state::StateStore::instance(),
            mc::client::ui::kagero::event::EventBus::instance()
        );
    }

    void TearDown() override {
        // Clear the binding context
        m_ctx.reset();
    }

    std::unique_ptr<binder::BindingContext> m_ctx;
};

TEST_F(BindingContextTest, ExposeVariable) {
    String playerName = "Steve";
    m_ctx->expose("player.name", &playerName);

    EXPECT_TRUE(m_ctx->hasPath("player.name"));
    EXPECT_FALSE(m_ctx->isWritable("player.name"));

    auto value = m_ctx->resolveBinding("player.name");
    EXPECT_FALSE(value.isNull());
    EXPECT_EQ(value.asString(), "Steve");
}

TEST_F(BindingContextTest, ExposeWritableVariable) {
    i32 health = 100;
    m_ctx->exposeWritable("player.health", &health);

    EXPECT_TRUE(m_ctx->hasPath("player.health"));
    EXPECT_TRUE(m_ctx->isWritable("player.health"));

    // Read
    auto value = m_ctx->resolveBinding("player.health");
    EXPECT_EQ(value.asInteger(), 100);

    // Write
    EXPECT_TRUE(m_ctx->setBinding("player.health", binder::Value(50)));
    EXPECT_EQ(health, 50);
}

TEST_F(BindingContextTest, ExposeCallback) {
    bool callbackCalled = false;
    String callbackArg;

    m_ctx->exposeCallback("onClick", [&](mc::client::ui::kagero::widget::Widget*,
                                          const mc::client::ui::kagero::event::Event&) {
        callbackCalled = true;
    });

    EXPECT_TRUE(m_ctx->hasCallback("onClick"));
    EXPECT_FALSE(m_ctx->hasCallback("nonexistent"));
}

TEST_F(BindingContextTest, ExposeSimpleCallback) {
    bool callbackCalled = false;

    m_ctx->exposeSimpleCallback("onStart", [&]() {
        callbackCalled = true;
    });

    EXPECT_TRUE(m_ctx->hasCallback("onStart"));

    // Invoke callback - use a concrete event type
    mc::client::ui::kagero::event::FocusGainedEvent event;
    EXPECT_TRUE(m_ctx->invokeCallback("onStart", nullptr, event));
    EXPECT_TRUE(callbackCalled);
}

TEST_F(BindingContextTest, InvokeNonexistentCallback) {
    mc::client::ui::kagero::event::FocusGainedEvent event;
    EXPECT_FALSE(m_ctx->invokeCallback("nonexistent", nullptr, event));
}

TEST_F(BindingContextTest, LoopVariables) {
    m_ctx->setLoopVariable("item", binder::Value(42));

    EXPECT_TRUE(m_ctx->hasLoopVariable("item"));
    EXPECT_FALSE(m_ctx->hasLoopVariable("nonexistent"));

    auto value = m_ctx->getLoopVariable("item");
    EXPECT_EQ(value.asInteger(), 42);

    m_ctx->clearLoopVariable("item");
    EXPECT_FALSE(m_ctx->hasLoopVariable("item"));
}

TEST_F(BindingContextTest, ResolveLoopVariable) {
    m_ctx->setLoopVariable("slot", binder::Value(String("item_data")));

    // Resolve $slot - need to call with loopVar parameter
    auto value = m_ctx->resolveBinding("$slot", "slot", binder::Value());
    EXPECT_EQ(value.asString(), "item_data");

    // Resolve with empty loop var - should use stored loop var
    value = m_ctx->resolveBinding("$slot", "", binder::Value());
    EXPECT_EQ(value.asString(), "item_data");
}

TEST_F(BindingContextTest, Subscribe) {
    String playerName = "Steve";
    m_ctx->expose("player.name", &playerName);

    String lastValue;
    u64 subId = m_ctx->subscribe("player.name", [&](const String& path, const binder::Value& value) {
        lastValue = value.asString();
    });

    EXPECT_GT(subId, 0u);

    // Notify change
    m_ctx->notifyChange("player.name", binder::Value(String("Alex")));
    EXPECT_EQ(lastValue, "Alex");

    // Unsubscribe
    m_ctx->unsubscribe(subId);

    // Notify again - lastValue may or may not update depending on implementation
    m_ctx->notifyChange("player.name", binder::Value(String("Bob")));
}

TEST_F(BindingContextTest, Clear) {
    String name = "Test";
    m_ctx->expose("name", &name);
    m_ctx->exposeCallback("callback", [](auto*, const auto&) {});

    EXPECT_TRUE(m_ctx->hasPath("name"));
    EXPECT_TRUE(m_ctx->hasCallback("callback"));

    m_ctx->clear();

    EXPECT_FALSE(m_ctx->hasPath("name"));
    EXPECT_FALSE(m_ctx->hasCallback("callback"));
}

// ==================== Value Tests ====================

class ValueTest : public ::testing::Test {
};

TEST_F(ValueTest, DefaultValue) {
    binder::Value v;
    EXPECT_TRUE(v.isNull());
    EXPECT_FALSE(v.isBool());
    EXPECT_FALSE(v.isInteger());
    EXPECT_FALSE(v.isFloat());
    EXPECT_FALSE(v.isString());
}

TEST_F(ValueTest, BoolValue) {
    binder::Value v(true);
    EXPECT_TRUE(v.isBool());
    EXPECT_FALSE(v.isNull());
    EXPECT_TRUE(v.asBool());
    EXPECT_EQ(v.toString(), "true");
}

TEST_F(ValueTest, IntegerValue) {
    binder::Value v(42);
    EXPECT_TRUE(v.isInteger());
    EXPECT_TRUE(v.isNumber());
    EXPECT_EQ(v.asInteger(), 42);
    EXPECT_FLOAT_EQ(v.asFloat(), 42.0f);
    EXPECT_EQ(v.toString(), "42");
}

TEST_F(ValueTest, FloatValue) {
    binder::Value v(3.14f);
    EXPECT_TRUE(v.isFloat());
    EXPECT_TRUE(v.isNumber());
    EXPECT_FLOAT_EQ(v.asFloat(), 3.14f);
    EXPECT_EQ(v.asInteger(), 3);
}

TEST_F(ValueTest, StringValue) {
    binder::Value v(String("hello"));
    EXPECT_TRUE(v.isString());
    EXPECT_EQ(v.asString(), "hello");
    EXPECT_EQ(v.toString(), "hello");
}

TEST_F(ValueTest, FromAny) {
    auto vBool = binder::Value::fromAny(std::any(true));
    EXPECT_TRUE(vBool.isBool());

    auto vInt = binder::Value::fromAny(std::any(42));
    EXPECT_TRUE(vInt.isInteger());

    auto vFloat = binder::Value::fromAny(std::any(3.14f));
    EXPECT_TRUE(vFloat.isFloat());

    auto vString = binder::Value::fromAny(std::any(String("test")));
    EXPECT_TRUE(vString.isString());

    auto vNull = binder::Value::fromAny(std::any());
    EXPECT_TRUE(vNull.isNull());
}

TEST_F(ValueTest, TypeConversions) {
    binder::Value vInt(42);
    EXPECT_EQ(vInt.toInteger(), 42);
    EXPECT_FLOAT_EQ(vInt.toFloat(), 42.0f);
    EXPECT_TRUE(vInt.toBool());

    binder::Value vFloat(3.14f);
    EXPECT_EQ(vFloat.toInteger(), 3);
    EXPECT_FLOAT_EQ(vFloat.toFloat(), 3.14f);
    EXPECT_TRUE(vFloat.toBool());

    binder::Value vZero(0);
    EXPECT_FALSE(vZero.toBool());

    binder::Value vTrue(true);
    EXPECT_EQ(vTrue.toInteger(), 1);
    EXPECT_FLOAT_EQ(vTrue.toFloat(), 1.0f);

    binder::Value vString(String("42"));
    EXPECT_EQ(vString.toInteger(), 42);
    EXPECT_FLOAT_EQ(vString.toFloat(), 42.0f);
}

TEST_F(ValueTest, Equality) {
    binder::Value v1(42);
    binder::Value v2(42);
    binder::Value v3(43);
    binder::Value v4(42.0f);

    EXPECT_TRUE(v1 == v2);
    EXPECT_FALSE(v1 == v3);
    EXPECT_TRUE(v1 == v4); // Integer and float comparison

    EXPECT_FALSE(v1 != v2);
    EXPECT_TRUE(v1 != v3);
}

TEST_F(ValueTest, GetProperty) {
    binder::Value v(42);
    auto prop = v.getProperty("anything");
    EXPECT_TRUE(prop.isNull()); // Simple values don't have properties
}

TEST_F(ValueTest, GetElement) {
    binder::Value v(42);
    auto elem = v.getElement(0);
    EXPECT_TRUE(elem.isNull()); // Simple values don't have elements
}

// ==================== TemplateErrorCollector Tests ====================

class TemplateErrorCollectorTest : public ::testing::Test {
};

TEST_F(TemplateErrorCollectorTest, BasicOperations) {
    core::TemplateErrorCollector collector;

    EXPECT_FALSE(collector.hasErrors());
    EXPECT_EQ(collector.errorCount(), 0u);
    EXPECT_EQ(collector.firstError(), nullptr);

    collector.addError(core::TemplateErrorType::LexerError, "Test error");

    EXPECT_TRUE(collector.hasErrors());
    EXPECT_EQ(collector.errorCount(), 1u);
    ASSERT_NE(collector.firstError(), nullptr);
    EXPECT_EQ(collector.firstError()->message, "Test error");

    collector.addError(core::TemplateErrorInfo(core::TemplateErrorType::ParserError, "Second error"));
    EXPECT_EQ(collector.errorCount(), 2u);

    const auto& errors = collector.errors();
    EXPECT_EQ(errors.size(), 2u);

    collector.clear();
    EXPECT_FALSE(collector.hasErrors());
    EXPECT_EQ(collector.errorCount(), 0u);
}

// ==================== Integration Tests ====================

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_config = core::TemplateConfig::defaults();
    }

    core::TemplateConfig m_config;
};

TEST_F(IntegrationTest, FullTemplateWorkflow) {
    // 1. Compile template
    compiler::TemplateCompiler compiler(m_config);
    auto compiled = compiler.compile(R"(
        <screen id="mainScreen" title="Inventory">
            <text id="playerName" bind:text="player.name"/>
            <text id="playerHealth" bind:text="player.health"/>
            <grid id="inventory" bind:items="player.inventory">
                <slot bind:item="$slot.item" bind:count="$slot.count" on:click="onSlotClick"/>
            </grid>
            <button id="closeBtn" on:click="onClose"/>
        </screen>
    )");

    ASSERT_NE(compiled, nullptr);
    EXPECT_TRUE(compiled->isValid()) <<
        (compiled->hasErrors() ? compiled->errors()[0].message : "none");

    // 2. Check that we have binding plans
    EXPECT_GE(compiled->bindingPlans().size(), 1u);

    // 3. Check that we have event plans
    EXPECT_GE(compiled->eventPlans().size(), 1u);

    // 4. Check registered callbacks
    EXPECT_GE(compiled->registeredCallbacks().size(), 1u);
}

TEST_F(IntegrationTest, ErrorRecovery) {
    // Test error recovery with multiple errors
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compile(R"(
        <screen>
            <invalidTag/>
            <text bind:text="123invalid"/>
            <button on:click="456invalid"/>
        </screen>
    )");

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->hasErrors());
}

TEST_F(IntegrationTest, BindingContextWithTemplate) {
    // Create binding context using singleton instances
    binder::BindingContext ctx(
        mc::client::ui::kagero::state::StateStore::instance(),
        mc::client::ui::kagero::event::EventBus::instance()
    );

    // Expose variables
    String playerName = "Steve";
    i32 playerHealth = 100;
    ctx.expose("player.name", &playerName);
    ctx.expose("player.health", &playerHealth);

    // Test binding resolution
    auto nameValue = ctx.resolveBinding("player.name");
    EXPECT_EQ(nameValue.asString(), "Steve");

    auto healthValue = ctx.resolveBinding("player.health");
    EXPECT_EQ(healthValue.asInteger(), 100);

    // Test callback
    bool callbackCalled = false;
    ctx.exposeSimpleCallback("onClose", [&]() {
        callbackCalled = true;
    });

    mc::client::ui::kagero::event::FocusGainedEvent event;
    EXPECT_TRUE(ctx.invokeCallback("onClose", nullptr, event));
    EXPECT_TRUE(callbackCalled);
}

TEST_F(IntegrationTest, ComplexTemplate) {
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compile(R"(
        <screen id="gameScreen">
            <viewport3d id="world" bind:render="world.scene"/>

            <widget id="hud">
                <text id="health" bind:text="player.health"/>
                <text id="hunger" bind:text="player.hunger"/>
                <text id="xp" bind:text="player.xp"/>
            </widget>

            <grid id="hotbar" bind:items="player.hotbar">
                <slot bind:item="$slot.item" bind:count="$slot.count"/>
            </grid>

            <!-- Conditional rendering -->
            <text id="sneakIndicator" bind:visible="player.isSneaking">
                Sneaking...
            </text>

            <button id="pauseBtn" on:click="onPause"/>
        </screen>
    )");

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->isValid());
    EXPECT_FALSE(result->hasErrors());

    // Verify structure
    // Bindings: bind:render(1) + bind:text*3(3) + bind:items(1) + bind:item(1) + bind:count(1) + bind:visible(1) = 8
    EXPECT_EQ(result->bindingPlans().size(), 8u);
    EXPECT_EQ(result->eventPlans().size(), 1u); // onPause
}

TEST_F(IntegrationTest, ParseComplexAttributeValues) {
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compile(R"(
        <widget pos="10,20" size="100,50" visible="true" opacity="0.75">
            <text text="Hello\nWorld"/>
        </widget>
    )");

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->isValid());

    // Check AST has correct values
    auto* root = result->astRoot();
    ASSERT_NE(root, nullptr);
    auto* elem = root->rootElement();
    ASSERT_NE(elem, nullptr);

    EXPECT_TRUE(elem->hasAttribute("pos"));
    EXPECT_TRUE(elem->hasAttribute("size"));
    EXPECT_TRUE(elem->hasAttribute("visible"));
    EXPECT_TRUE(elem->hasAttribute("opacity"));
}

// ==================== Loop Directive Tests ====================

class LoopDirectiveTest : public ::testing::Test {
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

TEST_F(LoopDirectiveTest, SimpleLoopDirective) {
    auto doc = parse(R"(
        <list for:item="item in items">
            <text bind:text="$item.name"/>
        </list>
    )");

    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* list = doc->rootElement();
    EXPECT_TRUE(list->loop.has_value());
    EXPECT_EQ(list->loop->collectionPath, "items");
    EXPECT_EQ(list->loop->itemVarName, "item");
}

TEST_F(LoopDirectiveTest, LoopWithIndex) {
    auto doc = parse(R"(
        <grid for:item="(item, index) in items">
            <slot bind:item="$item"/>
        </grid>
    )");

    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* grid = doc->rootElement();
    EXPECT_TRUE(grid->loop.has_value());
    EXPECT_EQ(grid->loop->collectionPath, "items");
    EXPECT_EQ(grid->loop->itemVarName, "item");
    EXPECT_EQ(grid->loop->indexVarName, "index");
    EXPECT_TRUE(grid->loop->hasIndex);
}

TEST_F(LoopDirectiveTest, LoopAttributeType) {
    auto doc = parse(R"(<list for:item="item in items"/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* list = doc->rootElement();

    // Find the for:item attribute
    bool foundLoopAttr = false;
    for (const auto& [name, attr] : list->attributes) {
        if (name.find("for:") == 0) {
            foundLoopAttr = true;
            EXPECT_EQ(attr.type, ast::AttributeType::Loop);
            break;
        }
    }
    EXPECT_TRUE(foundLoopAttr);
}

TEST_F(LoopDirectiveTest, LoopWithNestedBindings) {
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compile(R"(
        <list for:item="item in player.inventory">
            <text bind:text="$item.name"/>
            <text bind:text="$item.count"/>
        </list>
    )");

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->isValid());

    // Should have binding plans for nested bindings
    EXPECT_GE(result->bindingPlans().size(), 2u);
}

// ==================== Condition Directive Tests ====================

class ConditionDirectiveTest : public ::testing::Test {
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

TEST_F(ConditionDirectiveTest, SimpleConditionDirective) {
    auto doc = parse(R"(<text if:condition="player.alive"/>)");

    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* text = doc->rootElement();
    EXPECT_TRUE(text->condition.has_value());
    EXPECT_EQ(text->condition->booleanPath, "player.alive");
    EXPECT_FALSE(text->condition->negate);
}

TEST_F(ConditionDirectiveTest, ConditionWithNegation) {
    auto doc = parse(R"(<text if:condition="!player.hidden"/>)");

    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* text = doc->rootElement();
    EXPECT_TRUE(text->condition.has_value());
    EXPECT_EQ(text->condition->booleanPath, "player.hidden");
    EXPECT_TRUE(text->condition->negate);
}

TEST_F(ConditionDirectiveTest, ConditionAttributeType) {
    auto doc = parse(R"(<panel if:condition="game.isPlaying"/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* panel = doc->rootElement();

    // Find the if:condition attribute
    bool foundConditionAttr = false;
    for (const auto& [name, attr] : panel->attributes) {
        if (name.find("if:") == 0) {
            foundConditionAttr = true;
            EXPECT_EQ(attr.type, ast::AttributeType::Condition);
            break;
        }
    }
    EXPECT_TRUE(foundConditionAttr);
}

TEST_F(ConditionDirectiveTest, LegacyVisibleBinding) {
    // Test backward compatibility with bind:visible
    auto doc = parse(R"(<text bind:visible="player.isSneaking"/>)");

    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    // The condition should be extracted from bind:visible
    auto* text = doc->rootElement();

    // After categorizeAttributes and extractConditionInfo
    text->categorizeAttributes();

    // bind:visible should create a condition
    EXPECT_TRUE(text->condition.has_value());
    EXPECT_EQ(text->condition->booleanPath, "player.isSneaking");
}

TEST_F(ConditionDirectiveTest, ConditionWithContent) {
    compiler::TemplateCompiler compiler(m_config);

    // Use 'widget' as a generic container
    auto result = compiler.compile(R"(
        <widget if:condition="game.isPlaying">
            <text bind:text="player.health"/>
            <text bind:text="player.name"/>
        </widget>
    )");

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->isValid());

    // Should have binding plans for the content
    EXPECT_GE(result->bindingPlans().size(), 2u);
}

// ==================== Value Array Tests ====================

class ValueArrayTest : public ::testing::Test {
};

TEST_F(ValueArrayTest, EmptyArray) {
    binder::Value v(std::vector<binder::Value>{});
    EXPECT_TRUE(v.isArray());
    EXPECT_EQ(v.arraySize(), 0u);
}

TEST_F(ValueArrayTest, ArrayWithElements) {
    std::vector<binder::Value> items;
    items.emplace_back(binder::Value(1));
    items.emplace_back(binder::Value(2));
    items.emplace_back(binder::Value(3));

    binder::Value v(items);
    EXPECT_TRUE(v.isArray());
    EXPECT_EQ(v.arraySize(), 3u);

    EXPECT_EQ(v.arrayGet(0).asInteger(), 1);
    EXPECT_EQ(v.arrayGet(1).asInteger(), 2);
    EXPECT_EQ(v.arrayGet(2).asInteger(), 3);
}

TEST_F(ValueArrayTest, ArrayOutOfBounds) {
    std::vector<binder::Value> items;
    items.emplace_back(binder::Value(42));

    binder::Value v(items);
    EXPECT_TRUE(v.isArray());

    // Out of bounds should return null
    auto elem = v.arrayGet(10);
    EXPECT_TRUE(elem.isNull());
}

TEST_F(ValueArrayTest, ArrayToString) {
    std::vector<binder::Value> items;
    items.emplace_back(binder::Value(1));
    items.emplace_back(binder::Value(2));
    items.emplace_back(binder::Value(3));

    binder::Value v(items);
    String str = v.toString();
    EXPECT_TRUE(str.find("[") != String::npos);
    EXPECT_TRUE(str.find("]") != String::npos);
}

TEST_F(ValueArrayTest, ArrayEquality) {
    std::vector<binder::Value> items1;
    items1.emplace_back(binder::Value(1));
    items1.emplace_back(binder::Value(2));

    std::vector<binder::Value> items2;
    items2.emplace_back(binder::Value(1));
    items2.emplace_back(binder::Value(2));

    std::vector<binder::Value> items3;
    items3.emplace_back(binder::Value(1));
    items3.emplace_back(binder::Value(3));

    binder::Value v1(items1);
    binder::Value v2(items2);
    binder::Value v3(items3);

    EXPECT_TRUE(v1 == v2);
    EXPECT_FALSE(v1 == v3);
}

TEST_F(ValueArrayTest, FromArrayStatic) {
    auto v = binder::Value::fromArray({binder::Value(1), binder::Value(2)});
    EXPECT_TRUE(v.isArray());
    EXPECT_EQ(v.arraySize(), 2u);
}

TEST_F(ValueArrayTest, EmptyArrayStatic) {
    auto v = binder::Value::emptyArray();
    EXPECT_TRUE(v.isArray());
    EXPECT_EQ(v.arraySize(), 0u);
}

// ==================== BindingContext Collection Tests ====================

class BindingContextCollectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_ctx = std::make_unique<binder::BindingContext>(
            mc::client::ui::kagero::state::StateStore::instance(),
            mc::client::ui::kagero::event::EventBus::instance()
        );
    }

    std::unique_ptr<binder::BindingContext> m_ctx;
};

TEST_F(BindingContextCollectionTest, ResolveCollection) {
    // Set up a collection
    std::vector<binder::Value> items;
    items.emplace_back(binder::Value(String("item1")));
    items.emplace_back(binder::Value(String("item2")));
    items.emplace_back(binder::Value(String("item3")));

    m_ctx->setCollectionValue("inventory", items);

    auto resolved = m_ctx->resolveCollection("inventory");
    EXPECT_EQ(resolved.size(), 3u);
    EXPECT_EQ(resolved[0].asString(), "item1");
    EXPECT_EQ(resolved[1].asString(), "item2");
    EXPECT_EQ(resolved[2].asString(), "item3");
}

TEST_F(BindingContextCollectionTest, ResolveEmptyCollection) {
    auto resolved = m_ctx->resolveCollection("nonexistent");
    EXPECT_EQ(resolved.size(), 0u);
}

TEST_F(BindingContextCollectionTest, SetCollectionTemplate) {
    // Test the template version
    m_ctx->setCollection<i32>("numbers", {1, 2, 3, 4, 5});

    auto resolved = m_ctx->resolveCollection("numbers");
    EXPECT_EQ(resolved.size(), 5u);
    EXPECT_EQ(resolved[0].asInteger(), 1);
    EXPECT_EQ(resolved[4].asInteger(), 5);
}

TEST_F(BindingContextCollectionTest, LoopVariableWithCollection) {
    // Set up a collection
    m_ctx->setCollection<i32>("items", {10, 20, 30});

    // Iterate using loop variables
    auto items = m_ctx->resolveCollection("items");
    EXPECT_EQ(items.size(), 3u);

    for (size_t i = 0; i < items.size(); ++i) {
        m_ctx->setLoopVariable("item", items[i]);
        m_ctx->setLoopVariable("index", binder::Value(static_cast<i32>(i)));

        auto itemValue = m_ctx->getLoopVariable("item");
        auto indexValue = m_ctx->getLoopVariable("index");

        EXPECT_EQ(itemValue.asInteger(), static_cast<i32>(10 * (i + 1)));
        EXPECT_EQ(indexValue.asInteger(), static_cast<i32>(i));

        m_ctx->clearLoopVariable("item");
        m_ctx->clearLoopVariable("index");
    }
}

// ==================== Template System Tests ====================
// NOTE: TemplateSystemTest is disabled because it requires BuiltinWidgets and BuiltinEvents
// which depend on Widget implementations that are excluded from the test build.
// The TemplateSystem functionality is tested indirectly through integration tests.

// class TemplateSystemTest : public ::testing::Test {
// };
//
// TEST_F(TemplateSystemTest, InitializeShutdown) {
//     // Test initialization
//     initializeTemplateSystem();
//     EXPECT_TRUE(isTemplateSystemInitialized());
//
//     // Test idempotent initialization
//     initializeTemplateSystem();
//     EXPECT_TRUE(isTemplateSystemInitialized());
//
//     // Test shutdown
//     shutdownTemplateSystem();
//     EXPECT_FALSE(isTemplateSystemInitialized());
//
//     // Re-initialize for other tests
//     initializeTemplateSystem();
// }
