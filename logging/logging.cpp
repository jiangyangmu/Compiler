#include "logging.h"

#include <iostream>

namespace logging {

static inline const char *severity_to_string(int severity) {
  const char *s = "";
  switch (severity) {
  case INFO:
    s = "INFO";
    break;
  case WARNING:
    s = "WARNING";
    break;
  case ERROR:
    s = "ERROR";
    break;
  }
  return s;
}

class nullbuf : public std::streambuf {
public:
  int overflow(int c) { return c; }
};
static nullbuf null_buf;
static std::ostream null_stream(&null_buf);

std::ostream &log_null() {
  return null_stream;
}

std::ostream &log_console(int severity, const char *file, const char *line) {
  return std::cout << '[' << severity_to_string(severity) << ' ' << file << ':'
                   << line << ']' << ' ';
}

} // namespace logging
