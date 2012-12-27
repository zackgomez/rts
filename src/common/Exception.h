#ifndef SRC_COMMON_EXCEPTION_H_
#define SRC_COMMON_EXCEPTION_H_

#include <exception>
#include <string>

class exception_with_trace : public std::exception {
 public:
  explicit exception_with_trace(const std::string &msg = "");
  virtual ~exception_with_trace() throw() { }
  virtual const char *what() const throw();
 private:
  std::string msg_;
};

class file_exception : public exception_with_trace {
 public:
  explicit file_exception(const std::string &msg) :
    exception_with_trace(msg) {
  }
};

// Thrown from invariant
class violation_exception : public exception_with_trace {
 public:
  explicit violation_exception(const std::string &msg) :
    exception_with_trace(msg) {
  }
};

class engine_exception : public exception_with_trace {
 public:
  explicit engine_exception(const std::string &msg) :
    exception_with_trace(msg) {
  }
};

class network_exception : public exception_with_trace {
 public:
  explicit network_exception(const std::string &msg) :
    exception_with_trace(msg) {
  }
};

class timeout_exception : public exception_with_trace {
};

#endif  // SRC_COMMON_EXCEPTION_H_
