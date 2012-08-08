#pragma once
#include <string>
#include <exception>

/*
 * A variation of assert that throws an exception instead of calling abort.
 * The exception message includes the passed in message.
 * @param condition If false, will throw ViolationException
 * @param message A message describing the failure, when it occurs
 */
#define S(x) #x
#define S_(x) S(x)
#define S__LINE__ S_(__LINE__)
#define invariant(condition, message) __invariant(condition, \
    std::string() + __FILE__ ":" S__LINE__ " " #condition " - " + message)

void __invariant(bool condition, const std::string &message);

class exception_with_trace : public std::exception {
public:
  exception_with_trace(const std::string &msg);

  virtual const char *what() const throw();

private:
  std::string msg_;
};

// Thrown from invariant
class violation_exception : public exception_with_trace {
  violation_exception(const std::string &msg) :
    exception_with_trace(msg) {
  }
};
