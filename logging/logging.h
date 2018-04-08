#pragma once

#include <ostream>

namespace logging {

std::ostream & log_null();
std::ostream & log_console(int severity, const char * file, const char * line);

} // namespace logging

#define INFO 0
#define WARNING 1
#define ERROR 2

#define _STR(expr) #expr
#define STR(expr) (_STR(expr))

#define LOG(severity) \
    (logging::log_console((severity), __FILE__, STR(__LINE__)))

#define LOG_IF(severity, expr)                                          \
    ((expr) ? logging::log_console((severity), __FILE__, STR(__LINE__)) \
            : logging::log_null())

#ifdef DEBUG
#define DLOG(severity) LOG(severity)
#else
#define DLOG(severity) (logging::log_null())
#endif
