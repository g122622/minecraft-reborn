#pragma once

#include "Types.hpp"

#include <variant>
#include <string>
#include <optional>
#include <stdexcept>
#include <functional>

namespace mr {

// ============================================================================
// 错误类型
// ============================================================================

/**
 * @brief 错误代码枚举
 */
enum class ErrorCode : i32 {
    Success = 0,

    // 通用错误
    Unknown = -1,
    InvalidArgument = -2,
    NullPointer = -3,
    OutOfRange = -4,
    Overflow = -5,

    // 资源错误
    NotFound = -100,
    AlreadyExists = -101,
    ResourceExhausted = -102,
    OutOfMemory = -103,

    // 文件错误
    FileNotFound = -200,
    FileOpenFailed = -201,
    FileReadFailed = -202,
    FileWriteFailed = -203,
    FileCorrupted = -204,

    // 网络错误
    ConnectionFailed = -300,
    ConnectionClosed = -301,
    ConnectionTimeout = -302,
    InvalidPacket = -303,
    ProtocolError = -304,

    // 游戏错误
    InvalidBlock = -400,
    InvalidItem = -401,
    InvalidEntity = -402,
    InvalidPlayer = -403,
    InvalidWorld = -404,

    // 权限错误
    PermissionDenied = -500,
    Unauthorized = -501
};

/**
 * @brief 错误信息类
 */
class Error {
public:
    Error() = default;

    Error(ErrorCode code, StringView message = "", StringView source = "")
        : m_code(code)
        , m_message(message)
        , m_source(source)
    {
    }

    Error(ErrorCode code, const char* message, const char* source = "")
        : m_code(code)
        , m_message(message)
        , m_source(source)
    {
    }

    Error(ErrorCode code, String message, String source = "")
        : m_code(code)
        , m_message(std::move(message))
        , m_source(std::move(source))
    {
    }

    [[nodiscard]] ErrorCode code() const noexcept { return m_code; }
    [[nodiscard]] const String& message() const noexcept { return m_message; }
    [[nodiscard]] const String& source() const noexcept { return m_source; }

    [[nodiscard]] bool success() const noexcept
    {
        return m_code == ErrorCode::Success;
    }

    [[nodiscard]] bool failed() const noexcept
    {
        return m_code != ErrorCode::Success;
    }

    [[nodiscard]] String toString() const
    {
        if (m_source.empty()) {
            return formatError();
        }
        return m_source + ": " + formatError();
    }

    // 静态工厂方法
    static Error ok() { return Error(ErrorCode::Success); }

    static Error unknown(StringView message = "")
    {
        return Error(ErrorCode::Unknown, message);
    }

    static Error invalidArgument(StringView message = "")
    {
        return Error(ErrorCode::InvalidArgument, message);
    }

    static Error notFound(StringView message = "")
    {
        return Error(ErrorCode::NotFound, message);
    }

    static Error fileNotFound(StringView path = "")
    {
        return Error(ErrorCode::FileNotFound, path);
    }

    static Error connectionFailed(StringView message = "")
    {
        return Error(ErrorCode::ConnectionFailed, message);
    }

private:
    [[nodiscard]] String formatError() const
    {
        if (m_message.empty()) {
            return String("[Error ") + std::to_string(static_cast<i32>(m_code)) + "]";
        }
        return String("[Error ") + std::to_string(static_cast<i32>(m_code)) + "] " + m_message;
    }

    ErrorCode m_code = ErrorCode::Success;
    String m_message;
    String m_source;
};

// ============================================================================
// Result 类型
// ============================================================================

/**
 * @brief 结果类型 - 用于错误处理
 * @tparam T 成功时的值类型
 *
 * 使用示例:
 * @code
 * Result<int> divide(int a, int b) {
 *     if (b == 0) {
 *         return Error(ErrorCode::InvalidArgument, "Division by zero");
 *     }
 *     return a / b;
 * }
 *
 * auto result = divide(10, 2);
 * if (result.success()) {
 *     std::cout << "Result: " << result.value() << std::endl;
 * } else {
 *     std::cerr << result.error().toString() << std::endl;
 * }
 * @endcode
 */
template<typename T>
class Result {
public:
    // 构造函数
    Result() = delete;

    Result(T value) // NOLINT: 允许隐式转换
        : m_data(std::move(value))
    {
    }

    Result(Error error) // NOLINT: 允许隐式转换
        : m_data(std::move(error))
    {
    }

    // 拷贝和移动
    Result(const Result&) = default;
    Result(Result&&) noexcept = default;
    Result& operator=(const Result&) = default;
    Result& operator=(Result&&) noexcept = default;

    // 查询状态
    [[nodiscard]] bool success() const noexcept
    {
        return std::holds_alternative<T>(m_data);
    }

    [[nodiscard]] bool failed() const noexcept
    {
        return !success();
    }

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return success();
    }

    // 获取值
    [[nodiscard]] T& value() &
    {
        if (failed()) {
            throw std::runtime_error("Result contains error: " + error().toString());
        }
        return std::get<T>(m_data);
    }

    [[nodiscard]] const T& value() const&
    {
        if (failed()) {
            throw std::runtime_error("Result contains error: " + error().toString());
        }
        return std::get<T>(m_data);
    }

    [[nodiscard]] T&& value() &&
    {
        if (failed()) {
            throw std::runtime_error("Result contains error: " + error().toString());
        }
        return std::get<T>(std::move(m_data));
    }

    // 获取值或默认值
    [[nodiscard]] T valueOr(T defaultValue) const&
    {
        return success() ? value() : std::move(defaultValue);
    }

    [[nodiscard]] T valueOr(T defaultValue) &&
    {
        return success() ? std::move(value()) : std::move(defaultValue);
    }

    // 获取错误
    [[nodiscard]] const Error& error() const noexcept
    {
        return std::holds_alternative<Error>(m_data)
                   ? std::get<Error>(m_data)
                   : m_successError;
    }

    // 转换操作
    template<typename U>
    [[nodiscard]] Result<U> map(std::function<U(const T&)> f) const&
    {
        if (success()) {
            return f(value());
        }
        return error();
    }

    template<typename U>
    [[nodiscard]] Result<U> map(std::function<U(T)> f) &&
    {
        if (success()) {
            return f(std::move(value()));
        }
        return std::get<Error>(std::move(m_data));
    }

private:
    std::variant<T, Error> m_data;
    static inline Error m_successError{ErrorCode::Success};
};

// ============================================================================
// Result<void> 特化
// ============================================================================

template<>
class Result<void> {
public:
    Result()
        : m_success(true)
    {
    }

    Result(Error error) // NOLINT: 允许隐式转换
        : m_success(false)
        , m_error(std::move(error))
    {
    }

    // 拷贝和移动
    Result(const Result&) = default;
    Result(Result&&) noexcept = default;
    Result& operator=(const Result&) = default;
    Result& operator=(Result&&) noexcept = default;

    // 查询状态
    [[nodiscard]] bool success() const noexcept { return m_success; }
    [[nodiscard]] bool failed() const noexcept { return !m_success; }
    [[nodiscard]] explicit operator bool() const noexcept { return m_success; }

    // 获取错误
    [[nodiscard]] const Error& error() const noexcept { return m_error; }

    // 静态工厂方法
    static Result ok() { return Result(); }

private:
    bool m_success;
    Error m_error;
};

// ============================================================================
// 辅助宏
// ============================================================================

/**
 * @brief TRY宏 - 简化错误传播
 *
 * 使用示例:
 * @code
 * Result<int> foo() { ... }
 *
 * Result<void> bar() {
 *     TRY(auto value, foo());
 *     // 使用 value
 *     return Result<void>::ok();
 * }
 * @endcode
 */
#define MR_TRY(expr)               \
    do {                           \
        auto _result = (expr);     \
        if (_result.failed()) {    \
            return _result.error(); \
        }                          \
    } while (0)

#define MR_TRY_ASSIGN(var, expr) \
    do {                         \
        auto _result = (expr);   \
        if (_result.failed()) {  \
            return _result.error(); \
        }                        \
        var = std::move(_result.value()); \
    } while (0)

} // namespace mr
