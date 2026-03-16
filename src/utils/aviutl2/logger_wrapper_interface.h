#ifndef LOGGER_WRAPPER_INTERFACE_H
#define LOGGER_WRAPPER_INTERFACE_H

#include <format>
#include <string>
#include <string_view>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "aviutl2_sdk/logger2.h"

class LoggerWrapperInterface {
public:
    virtual ~LoggerWrapperInterface() = default;

    // log
    template <typename... Args>
    void log(std::string_view fmt, Args... args) const
    {
        log_impl(toWideString(std::vformat(fmt, std::make_format_args(args...))).c_str());
    }
    template <typename... Args>
    void log(std::wstring_view fmt, Args&&... args) const
    {
        log_impl(std::vformat(fmt, std::make_wformat_args(args...)));
    }

    // info
    template <typename... Args>
    void info(std::string_view fmt, Args... args) const
    {
        info_impl(toWideString(std::vformat(fmt, std::make_format_args(args...))).c_str());
    }
    template <typename... Args>
    void info(std::wstring_view fmt, Args&&... args) const
    {
        info_impl(std::vformat(fmt, std::make_wformat_args(args...)));
    }

    // warn
    template <typename... Args>
    void warn(std::string_view fmt, Args... args) const
    {
        warn_impl(toWideString(std::vformat(fmt, std::make_format_args(args...))).c_str());
    }
    template <typename... Args>
    void warn(std::wstring_view fmt, Args&&... args) const
    {
        warn_impl(std::vformat(fmt, std::make_wformat_args(args...)));
    }

    // error
    template <typename... Args>
    void error(std::string_view fmt, Args... args) const
    {
        error_impl(toWideString(std::vformat(fmt, std::make_format_args(args...))).c_str());
    }
    template <typename... Args>
    void error(std::wstring_view fmt, Args&&... args) const
    {
        error_impl(std::vformat(fmt, std::make_wformat_args(args...)));
    }

    // verbose
    template <typename... Args>
    void verbose(std::string_view fmt, Args... args) const
    {
        verbose_impl(toWideString(std::vformat(fmt, std::make_format_args(args...))).c_str());
    }
    template <typename... Args>
    void verbose(std::wstring_view fmt, Args&&... args) const
    {
        verbose_impl(std::vformat(fmt, std::make_wformat_args(args...)));
    }

private:
    virtual void log_impl(std::wstring_view msg) const     = 0;
    virtual void info_impl(std::wstring_view msg) const    = 0;
    virtual void warn_impl(std::wstring_view msg) const    = 0;
    virtual void error_impl(std::wstring_view msg) const   = 0;
    virtual void verbose_impl(std::wstring_view msg) const = 0;

    [[nodiscard]] static std::wstring toWideString(std::string_view str, uint32_t code_page = CP_ACP)
    {
        if (str.empty()) return {};

        const int size_needed = ::MultiByteToWideChar(code_page, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);
        if (size_needed <= 0) return {};

        std::wstring result(size_needed, L'\0');
        ::MultiByteToWideChar(code_page, 0, str.data(), static_cast<int>(str.size()), result.data(), size_needed);

        return result;
    }
};

LoggerWrapperInterface* get_logger_wrapper_interface(LOG_HANDLE* log_handle);

extern LoggerWrapperInterface* logger_wrapper_instance;

#endif
