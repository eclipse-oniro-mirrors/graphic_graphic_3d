/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_CORE_ILOGGER_H
#define API_CORE_ILOGGER_H

#include <base/containers/string_view.h>
#include <base/containers/unique_ptr.h>
#include <base/util/uid.h>
#include <core/namespace.h>
#include <core/plugin/intf_interface.h>

// Enabling warnings checks for format string parameters.
#if defined(__clang__)
#define FORMAT_FUNC(formatPos, argsPos) __attribute__((__format__(__printf__, formatPos, argsPos)))
#define FORMAT_ATTRIBUTE
#define CHECK_FORMAT_STRING(...)
#elif defined(__GNUC__)
#define FORMAT_FUNC(formatPos, argsPos) __attribute__((format(printf, formatPos, argsPos)))
#define FORMAT_ATTRIBUTE
#define CHECK_FORMAT_STRING(...)
#elif defined(_MSC_VER)
#define FORMAT_FUNC(...)
#define FORMAT_ATTRIBUTE _In_z_ _Printf_format_string_
// Hack to force format string verifying during compile time. see
// https://devblogs.microsoft.com/cppblog/format-specifiers-checking/ Note that the snprintf should never be evaluated
// during runtime due to the use of "false &&" (dead-code elimination works on debug too)
#define CHECK_FORMAT_STRING(...) (false && _snprintf_s(nullptr, 0, 0, ##__VA_ARGS__));
#else
#define FORMAT_FUNC
#define FORMAT_ATTRIBUTE
#define CHECK_FORMAT_STRING(...)
#endif

CORE_BEGIN_NAMESPACE()
/** @ingroup group_log */
/** Logger */
class ILogger : public IInterface {
public:
#ifndef SWIG
    static constexpr auto UID = BASE_NS::Uid { "d9c55b07-441c-4059-909b-88ebc3c07b1e" };
#endif

    /** Logging level */
    enum class LogLevel {
        /** Verbose */
        LOG_VERBOSE = 0,
        /** Debug */
        LOG_DEBUG,
        /** Info */
        LOG_INFO,
        /** Warning */
        LOG_WARNING,
        /** Error */
        LOG_ERROR,
        /** Fatal */
        LOG_FATAL,

        // This level should only be used when setting the log level filter with SetLogLevel(),
        // not as a log level for a message.
        /** None */
        LOG_NONE,
    };

    /** Output */
    class IOutput {
    public:
        /** Write */
        virtual void Write(LogLevel logLevel, const BASE_NS::string_view filename, int lineNumber,
            const BASE_NS::string_view message) = 0;

        struct Deleter {
            constexpr Deleter() noexcept = default;
            void operator()(IOutput* ptr) const
            {
                ptr->Destroy();
            }
        };
        using Ptr = BASE_NS::unique_ptr<IOutput, Deleter>;

    protected:
        IOutput() = default;
        virtual ~IOutput() = default;
        virtual void Destroy() = 0;
    };

    /** Write to log (Version of logger that takes va_list, please use macros instead) */
    virtual void VLog(LogLevel logLevel, const BASE_NS::string_view filename, int lineNumber,
        const BASE_NS::string_view format, va_list args) = 0;
    /** Write to log once (Version of logger that takes va_list, please use macros instead) */
    virtual void VLogOnce(const BASE_NS::string_view id, LogLevel logLevel, const BASE_NS::string_view filename,
        int lineNumber, const BASE_NS::string_view format, va_list args) = 0;
    virtual bool VLogAssert(const BASE_NS::string_view filename, int lineNumber, bool expression,
        const BASE_NS::string_view expressionString, const BASE_NS::string_view format, va_list args) = 0;

    /** Write to log (Takes var args) */
    virtual FORMAT_FUNC(5, 6) void Log(LogLevel logLevel, const BASE_NS::string_view filename, int lineNumber,
        FORMAT_ATTRIBUTE const char* format, ...) = 0;
    /** Write to log (Version which is used with asserts) */
    virtual FORMAT_FUNC(6, 7) bool LogAssert(const BASE_NS::string_view filename, int lineNumber, bool expression,
        const BASE_NS::string_view expressionString, FORMAT_ATTRIBUTE const char* format, ...) = 0;

    /** Get log level */
    virtual LogLevel GetLogLevel() const = 0;
    /** Set log level */
    virtual void SetLogLevel(LogLevel logLevel) = 0;

    /** Add output (for custom loggers) */
    virtual void AddOutput(IOutput::Ptr output) = 0;

    /** resets all "logOnce" handles. allowing the messages to be logged again */
    virtual void CheckOnceReset() = 0;

protected:
    ILogger() = default;
    virtual ~ILogger() = default;

    ILogger(ILogger const&) = delete;
    void operator=(ILogger const&) = delete;
};

inline constexpr const BASE_NS::string_view GetName(const ILogger*)
{
    return "ILogger";
}

/** @} */
CORE_END_NAMESPACE()
#endif // API_CORE_ILOGGER_H
