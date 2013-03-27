#include "common/Logger.h"

static const size_t MAX_MESSAGE_LENGTH = 512;
Logger * Logger::instance_ = nullptr;

Logger::Logger() {
}

void Logger::initLogger() {
  invariant(!instance_, "Logger already initialized");
  instance_ = new Logger();
  atexit(&Logger::shutdownLogger);
}

void Logger::shutdownLogger() {
  delete instance_;
}

void Logger::log(const char * format, ...) {
  // TODO(zack): Move the string processing to the worker thread.
  va_list va;
  char *buf = new char[MAX_MESSAGE_LENGTH];

  va_start(va, format);
  vsnprintf(buf, MAX_MESSAGE_LENGTH, format, va);
  va_end(va);

  worker_.post([=]() {
    printf("%s", buf);
    delete buf;
  });
}
