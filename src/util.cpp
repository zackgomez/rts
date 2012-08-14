#include "util.h"

void __invariant(bool condition, const std::string &message) {
  if (!condition) {
    throw violation_exception(message);
  }
}
