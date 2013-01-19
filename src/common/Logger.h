#ifndef SRC_COMMON_LOGGER_H_
#define SRC_COMMON_LOGGER_H_

#include <memory>
#include <iostream>
#include <string>

#define LOG(str) std::cout << __FILE__ << ":" <<  __LINE__ << " "#str << " - "

class Logger;
typedef std::auto_ptr<Logger> LoggerPtr;

class Logger {
 public:
  static LoggerPtr getLogger(const std::string &prefix) {
    LoggerPtr ret(new Logger(prefix));
    return ret;
  }

  std::ostream& log() {
    std::cout << prefix_ << ": ";
    return std::cout;
  }

  std::ostream& debug() {
    std::cout << "DEBUG - " << prefix_ << ": ";
    return std::cerr;
  }
  std::ostream& info() {
    std::cout << "INFO - " << prefix_ << ": ";
    return std::cout;
  }
  std::ostream& warning() {
    std::cerr << "WARNING - " << prefix_ << ": ";
    return std::cerr;
  }
  std::ostream& error() {
    std::cerr << "ERROR - " << prefix_ << ": ";
    return std::cerr;
  }

  std::ostream& fatal() {
    std::cerr << "ERROR - " << prefix_ << ": ";
    return std::cerr;
  }

 private:
  explicit Logger(const std::string &prefix) :
    prefix_(prefix) {
    /* Empty */
  }

  std::string prefix_;
};

#endif  // SRC_COMMON_LOGGER_H_
