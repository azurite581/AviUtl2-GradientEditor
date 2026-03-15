#ifndef LOGGER_WRAPPER_H
#define LOGGER_WRAPPER_H

#include <format>
#include <string>
#include <utility>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "logger2.h"

class LoggerWrapper {
public:
    LoggerWrapper() = default;

    explicit LoggerWrapper(LOG_HANDLE* log_handle)
        : m_log_handle(log_handle)
    {
    }

    void setLogHandle(LOG_HANDLE* logHandle) noexcept { m_log_handle = logHandle; }

    [[nodiscard]] LOG_HANDLE* getLogHandle() const noexcept { return m_log_handle; }

    // log
    template <typename... Args>
    void log(std::wstring_view fmt, Args... args) const
    {
        m_log_handle->log(m_log_handle, std::vformat(fmt, std::make_wformat_args(args...)).c_str());
    }

    template <typename... Args>
    void log(std::string_view fmt, Args... args) const
    {
        m_log_handle->log(m_log_handle, toWideString(std::vformat(fmt, std::make_format_args(args...))).c_str());
    }

    // info
    template <typename... Args>
    void info(std::wstring_view fmt, Args... args) const
    {
        m_log_handle->info(m_log_handle, std::vformat(fmt, std::make_wformat_args(args...)).c_str());
    }

    template <typename... Args>
    void info(std::string_view fmt, Args... args) const
    {
        m_log_handle->info(m_log_handle, toWideString(std::vformat(fmt, std::make_format_args(args...))).c_str());
    }

    // warn
    template <typename... Args>
    void warn(std::wstring_view fmt, Args... args) const
    {
        m_log_handle->warn(m_log_handle, std::vformat(fmt, std::make_wformat_args(args...)).c_str());
    }

    template <typename... Args>
    void warn(std::string_view fmt, Args... args) const
    {
        m_log_handle->warn(m_log_handle, toWideString(std::vformat(fmt, std::make_format_args(args...))).c_str());
    }

    // error
    template <typename... Args>
    void error(std::wstring_view fmt, Args... args) const
    {
        m_log_handle->error(m_log_handle, std::vformat(fmt, std::make_wformat_args(args...)).c_str());
    }

    template <typename... Args>
    void error(std::string_view fmt, Args... args) const
    {
        m_log_handle->error(m_log_handle, toWideString(std::vformat(fmt, std::make_format_args(args...))).c_str());
    }

    // verbose
    template <typename... Args>
    void verbose(std::wstring_view fmt, Args... args) const
    {
        m_log_handle->verbose(m_log_handle, std::vformat(fmt, std::make_wformat_args(args...)).c_str());
    }

    template <typename... Args>
    void verbose(std::string_view fmt, Args... args) const
    {
        m_log_handle->verbose(m_log_handle, toWideString(std::vformat(fmt, std::make_format_args(args...))).c_str());
    }

private:
    LOG_HANDLE* m_log_handle = nullptr;

    // ログのエンコードが Shift_JIS なのでデフォルトのコードページは CP_ACP (ANSIコードページ) にする
    [[nodiscard]] static std::wstring toWideString(std::string_view str, uint32_t code_page = CP_ACP)
    {
        if (str.empty()) {
            return {};
        }

        const int size_needed = ::MultiByteToWideChar(
            code_page, 0,
            str.data(), static_cast<int>(str.size()),
            nullptr, 0);

        if (size_needed <= 0) {
            return {};
        }

        std::wstring result(size_needed, L'\0');
        ::MultiByteToWideChar(
            code_page, 0,
            str.data(), static_cast<int>(str.size()),
            result.data(), size_needed);

        return result;
    }
};

#endif  // LOGGER_WRAPPER_H
