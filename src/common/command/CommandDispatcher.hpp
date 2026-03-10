#pragma once

#include "common/core/Types.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/CommandContext.hpp"
#include "common/command/StringReader.hpp"
#include "common/command/CommandResult.hpp"
#include <memory>
#include <functional>
#include <vector>
#include <unordered_set>

namespace mr::command {

// ========== 类型别名 ==========

/// 命令执行回调
template<typename S>
using CommandCallback = std::function<i32(CommandContext<S>&)>;

/// 权限检查谓词
template<typename S>
using RequirementPredicate = std::function<bool(const S&)>;

// ========== 前向声明 ==========

template<typename S>
class LiteralArgumentBuilder;

template<typename S, typename T>
class RequiredArgumentBuilder;

// ========== 分发器 ==========

/**
 * @brief 命令分发器
 *
 * 核心命令解析和执行引擎。
 *
 * 功能：
 * - 注册命令节点
 * - 解析命令字符串
 * - 执行命令
 * - 提供自动补全建议
 *
 * 使用示例：
 * @code
 * CommandDispatcher<CommandSource> dispatcher;
 *
 * // 注册命令
 * auto node = std::make_shared<LiteralCommandNode<CommandSource>>("gamemode");
 * node->setCommand([](CommandContext<CommandSource>&) { return 1; });
 * dispatcher.registerCommand(node);
 *
 * // 执行命令
 * auto result = dispatcher.execute("gamemode", source);
 * @endcode
 */
template<typename S>
class CommandDispatcher {
public:
    using NodePtr = std::shared_ptr<CommandNode<S>>;
    using RootNodePtr = std::shared_ptr<RootCommandNode<S>>;

    CommandDispatcher()
        : m_root(std::make_shared<RootCommandNode<S>>()) {}

    // ========== 命令注册 ==========

    /**
     * @brief 注册命令节点
     * @param node 命令节点
     * @return 注册的命令节点
     */
    NodePtr registerCommand(std::shared_ptr<LiteralCommandNode<S>> node) {
        m_root->addChild(node);
        return node;
    }

    // ========== 命令解析 ==========

    /**
     * @brief 解析命令字符串
     * @param input 命令字符串（可带或不带 / 前缀）
     * @param source 命令源
     * @return 解析结果
     */
    [[nodiscard]] ParseResults<S> parse(StringView input, S& source) const {
        StringReader reader(input);

        // 跳过命令前缀
        if (reader.canRead() && reader.peek() == '/') {
            reader.skip();
        }

        auto context = std::make_unique<CommandContext<S>>(
            source, input, m_root);

        return parseNodes(reader, m_root, std::move(context));
    }

    /**
     * @brief 解析命令节点
     */
    [[nodiscard]] ParseResults<S> parseNodes(
        StringReader& reader,
        NodePtr node,
        std::unique_ptr<CommandContext<S>> context
    ) const;

    // ========== 命令执行 ==========

    /**
     * @brief 执行命令
     * @param input 命令字符串
     * @param source 命令源
     * @return 执行结果
     */
    Result<CommandResult> execute(StringView input, S& source) {
        ParseResults<S> parseResult = parse(input, source);
        return execute(parseResult);
    }

    /**
     * @brief 执行已解析的命令
     * @param parse 解析结果
     * @return 执行结果
     */
    Result<CommandResult> execute(ParseResults<S>& parse);

    // ========== 命令查询 ==========

    /**
     * @brief 获取根节点
     */
    [[nodiscard]] NodePtr getRoot() const noexcept { return m_root; }

    /**
     * @brief 获取命令路径
     * @param node 命令节点
     * @return 路径字符串列表
     */
    [[nodiscard]] std::vector<String> getPath(NodePtr node) const;

    /**
     * @brief 查找歧义命令
     */
    void findAmbiguities(
        std::function<void(NodePtr, NodePtr, const std::set<String>&)> callback
    ) const;

private:
    /**
     * @brief 执行命令节点
     */
    i32 executeCommand(
        const CommandContext<S>& context,
        NodePtr node,
        StringReader& reader
    );

    RootNodePtr m_root;
};

// ========== 构建器工具函数 ==========

/**
 * @brief 创建字面量构建器
 */
template<typename S>
LiteralArgumentBuilder<S> literal(const String& name);

/**
 * @brief 创建参数构建器
 */
template<typename S, typename T>
RequiredArgumentBuilder<S, T> argument(const String& name, std::shared_ptr<ArgumentCommandNode<S, T>> type);

// ========== 字面量构建器 ==========

/**
 * @brief 字面量构建器
 */
template<typename S>
class LiteralArgumentBuilder {
public:
    using NodePtr = std::shared_ptr<CommandNode<S>>;

    explicit LiteralArgumentBuilder(const String& literal)
        : m_literal(literal) {}

    // ========== 命令执行 ==========

    LiteralArgumentBuilder& executes(CommandCallback<S> command) {
        m_command = std::move(command);
        return *this;
    }

    // ========== 权限 ==========

    LiteralArgumentBuilder& requires(RequirementPredicate<S> requirement) {
        m_requirement = std::move(requirement);
        return *this;
    }

    // ========== 子节点 ==========

    LiteralArgumentBuilder& then(NodePtr node) {
        m_children.push_back(node);
        return *this;
    }

    template<typename Builder>
    LiteralArgumentBuilder& then(Builder&& builder) {
        m_children.push_back(builder.build());
        return *this;
    }

    // ========== 重定向 ==========

    LiteralArgumentBuilder& redirectsTo(NodePtr target) {
        m_redirect = target;
        return *this;
    }

    // ========== 构建 ==========

    [[nodiscard]] NodePtr build() const {
        auto node = std::make_shared<LiteralCommandNode<S>>(m_literal);

        if (m_command) {
            node->setCommand(m_command);
        }
        node->setRequirement(m_requirement);

        for (const auto& child : m_children) {
            node->addChild(child);
        }

        if (m_redirect) {
            node->setRedirect(m_redirect);
        }

        return node;
    }

private:
    String m_literal;
    CommandCallback<S> m_command;
    RequirementPredicate<S> m_requirement = [](const S&) { return true; };
    std::vector<NodePtr> m_children;
    NodePtr m_redirect;
};

// ========== 参数构建器 ==========

/**
 * @brief 参数构建器
 */
template<typename S, typename T>
class RequiredArgumentBuilder {
public:
    using NodePtr = std::shared_ptr<CommandNode<S>>;

    RequiredArgumentBuilder(const String& name, std::shared_ptr<ArgumentCommandNode<S, T>> type)
        : m_name(name)
        , m_type(type) {}

    // ========== 命令执行 ==========

    RequiredArgumentBuilder& executes(CommandCallback<S> command) {
        m_command = std::move(command);
        return *this;
    }

    // ========== 权限 ==========

    RequiredArgumentBuilder& requires(RequirementPredicate<S> requirement) {
        m_requirement = std::move(requirement);
        return *this;
    }

    // ========== 子节点 ==========

    RequiredArgumentBuilder& then(NodePtr node) {
        m_children.push_back(node);
        return *this;
    }

    template<typename Builder>
    RequiredArgumentBuilder& then(Builder&& builder) {
        m_children.push_back(builder.build());
        return *this;
    }

    // ========== 构建 ==========

    [[nodiscard]] NodePtr build() const {
        if (m_command) {
            m_type->setCommand(m_command);
        }
        m_type->setRequirement(m_requirement);

        for (const auto& child : m_children) {
            m_type->addChild(child);
        }

        return m_type;
    }

    /**
     * @brief 设置建议提供器
     */
    template<typename Provider>
    RequiredArgumentBuilder& suggests(Provider /*provider*/) {
        // TODO: 实现建议提供器
        return *this;
    }

private:
    String m_name;
    std::shared_ptr<ArgumentCommandNode<S, T>> m_type;
    CommandCallback<S> m_command;
    RequirementPredicate<S> m_requirement = [](const S&) { return true; };
    std::vector<NodePtr> m_children;
};

// ========== 模板实现 ==========

template<typename S>
LiteralArgumentBuilder<S> literal(const String& name) {
    return LiteralArgumentBuilder<S>(name);
}

template<typename S, typename T>
RequiredArgumentBuilder<S, T> argument(const String& name, std::shared_ptr<ArgumentCommandNode<S, T>> type) {
    return RequiredArgumentBuilder<S, T>(name, type);
}

// ========== CommandDispatcher 模板实现 ==========

template<typename S>
ParseResults<S> CommandDispatcher<S>::parseNodes(
    StringReader& reader,
    NodePtr node,
    std::unique_ptr<CommandContext<S>> context
) const {
    // 跳过空白
    reader.skipWhitespace();

    // 如果没有更多输入，返回当前上下文
    if (!reader.canRead()) {
        return ParseResults<S>(std::move(context), reader.getRemaining());
    }

    // 尝试匹配子节点
    std::vector<NodePtr> matchedChildren;
    std::vector<ParseResults<S>> potentialResults;

    for (const auto& [name, child] : node->getChildren()) {
        if (!child->canUse(context->getSource())) {
            continue;
        }

        StringReader childReader = reader;  // 复制读取器
        i32 startCursor = childReader.getCursor();

        // 尝试解析
        bool matched = false;
        try {
            if (child->getType() == NodeType::Literal) {
                // 字面量匹配
                String literal = childReader.readUnquotedString();
                if (literal == name) {
                    matched = true;
                } else {
                    childReader.setCursor(startCursor);
                }
            } else if (child->getType() == NodeType::Argument) {
                // 参数节点 - 总是匹配（解析在后面进行）
                matched = true;
            }
        } catch (...) {
            childReader.setCursor(startCursor);
        }

        if (matched) {
            // 创建新上下文
            auto childContext = std::make_unique<CommandContext<S>>(
                context->getSource(),
                context->getInput(),
                context->getRootNode()
            );

            childContext->setCurrentNode(child);

            matchedChildren.push_back(child);
            potentialResults.push_back(parseNodes(childReader, child, std::move(childContext)));
        }
    }

    // 处理匹配结果
    if (potentialResults.empty()) {
        // 没有匹配的节点
        return ParseResults<S>(
            CommandException(CommandErrorType::DispatcherUnknownCommand,
                "Unknown command", reader.getCursor()),
            reader.getCursor()
        );
    }

    if (potentialResults.size() == 1) {
        return std::move(potentialResults[0]);
    }

    // 多个匹配 - 选择最长的匹配
    return std::move(potentialResults[0]);
}

template<typename S>
Result<CommandResult> CommandDispatcher<S>::execute(ParseResults<S>& parse) {
    if (parse.isFailure()) {
        return Error(ErrorCode::Unknown, parse.getError()->message());
    }

    auto* context = parse.getContext();
    if (!context) {
        return Error(ErrorCode::Unknown, "No command context");
    }

    auto node = context->getCurrentNode();
    if (!node || !node->hasCommand()) {
        return Error(ErrorCode::Unknown, "No command to execute");
    }

    try {
        i32 result = node->getCommand()(*context);
        return CommandResult::success(result);
    } catch (const CommandException& e) {
        return Error(ErrorCode::Unknown, e.message());
    } catch (const std::exception& e) {
        return Error(ErrorCode::Unknown, e.what());
    }
}

template<typename S>
std::vector<String> CommandDispatcher<S>::getPath(NodePtr /*node*/) const {
    // TODO: 实现路径查找
    return {};
}

template<typename S>
void CommandDispatcher<S>::findAmbiguities(
    std::function<void(NodePtr, NodePtr, const std::set<String>&)> /*callback*/
) const {
    // TODO: 实现歧义检测
}

} // namespace mr::command
