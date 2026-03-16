#ifndef LOGGER_WRAPPER_H
#define LOGGER_WRAPPER_H

#include "logger_wrapper_interface.h"
#include "aviutl2_sdk/logger2.h"

class LoggerWrapper : public LoggerWrapperInterface {
public:
    LoggerWrapper(LOG_HANDLE* log_handle) : m_log_handle{log_handle} {}

    LoggerWrapper(LoggerWrapper const&) = delete;
	LoggerWrapper& operator=(LoggerWrapper const&) = delete;
	LoggerWrapper(LoggerWrapper&&) = delete;
	LoggerWrapper& operator=(LoggerWrapper&&) = delete;

private:
    void log_impl(std::wstring_view msg) const override { m_log_handle->log(m_log_handle, msg.data()); }
    void info_impl(std::wstring_view msg) const override { m_log_handle->info(m_log_handle, msg.data()); }
    void warn_impl(std::wstring_view msg) const override { m_log_handle->warn(m_log_handle, msg.data()); }
    void error_impl(std::wstring_view msg) const override { m_log_handle->error(m_log_handle, msg.data()); }
    void verbose_impl(std::wstring_view msg) const override { m_log_handle->verbose(m_log_handle, msg.data()); }

    LOG_HANDLE* m_log_handle = nullptr;
};

#endif  // LOGGER_WRAPPER_H
