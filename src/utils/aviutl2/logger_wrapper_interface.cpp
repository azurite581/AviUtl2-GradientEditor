#include "logger_wrapper_interface.h"

#include "logger_wrapper.h"

LoggerWrapperInterface* logger_wrapper_instance = nullptr;

LoggerWrapperInterface* get_logger_wrapper_interface(LOG_HANDLE* log_handle)
{
    static bool init = [log_handle]() {
        if (!logger_wrapper_instance) {
            static LoggerWrapper logger_wrapper(log_handle);
            logger_wrapper_instance = &logger_wrapper;
        }
        return true;
    }();

    return logger_wrapper_instance;
}
