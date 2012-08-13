#include <stdexcept>
#ifndef _MSC_VER
#include <execinfo.h>
#include <string>
#include <sstream>
#include <cxxabi.h>
#include <cstring>
#endif
#include "util.h"

// @param ignore the number of traces to ignore
std::string get_backtrace(size_t ignore = 0);

void __invariant(bool condition, const std::string &message) {
  if (!condition) {
    throw violation_exception(message);
  }
}

exception_with_trace::exception_with_trace(const std::string &msg) {
  msg_ = msg + "\nStack trace:\n" + get_backtrace(1) + "\n";
}

const char * exception_with_trace::what() const throw() {
  return msg_.c_str();
}

#ifndef _MSC_VER
std::string get_backtrace(size_t ignore) {

  const size_t max_depth = 100;
  size_t stack_depth;
  void *stack_addrs[max_depth];
  char **stack_strings;

  stack_depth = backtrace(stack_addrs, max_depth);
  stack_strings = backtrace_symbols(stack_addrs, stack_depth);

  std::stringstream ss;

  for (size_t i = 1 + ignore; i < stack_depth; i++) {
    size_t sz = 200; // just a guess, template names will go much wider
    char *function = static_cast<char*>(malloc(sz));
    char *begin = 0, *end = 0;
    // find the parentheses and address offset surrounding the mangled name
    for (char *j = stack_strings[i]; *j; ++j) {
      if (*j == '(') {
        begin = j;
      } else if (*j == '+') {
        end = j;
      }
    }
    if (begin && end) {
      *begin++ = '\0';
      *end = '\0';
      // found our mangled name, now in [begin, end)

      int status;
      char *ret = abi::__cxa_demangle(begin, function, &sz, &status);
      if (ret) {
        // return value may be a realloc() of the input
        function = ret;
      } else {
        // demangling failed, just pretend it's a C function with no args
        std::strncpy(function, begin, sz);
        std::strncat(function, "()", sz);
        function[sz-1] = '\0';
      }
      ss << "    " << stack_strings[i] << ":" << function << '\n';
    } else {
      // didn't find the mangled name, just print the whole line
      ss << "    " << stack_strings[i] << '\n';
    }
    free(function);
  }

  free(stack_strings); // malloc()ed by backtrace_symbols

  return ss.str();
}
#else
std::string get_backtrace(size_t ignore) {
  // TODO do this for windows
  return "NOT IMPLEMENTED ON WINDOWS\n";
}
#endif
