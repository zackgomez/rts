#ifndef SRC_COMMON_LOGGER_H_
#define SRC_COMMON_LOGGER_H_

#include <iostream>
#include <fstream>
#include <string>
#include <boost/format.hpp>
#include "common/util.h"
#include "common/WorkerThread.h"

#define LOG(str) std::cout << __FILE__ << ":" <<  __LINE__ << " "#str << " - "

class Logger;
typedef std::auto_ptr<Logger> LoggerPtr;

class Logger {
 public:
  static void initLogger();
  static void shutdownLogger();

  static Logger* get() {
    invariant(instance_, "Logging not initialized");
    return instance_;
  }

  void log(const char * format, ...);


 private:
  Logger();
  static Logger *instance_;

  //std::ofstream file_;
  WorkerThread worker_;
};

#endif  // SRC_COMMON_LOGGER_H_
