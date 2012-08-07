#include <stdexcept>
#include "util.h"

void __invariant(bool condition, const std::string &message) {
  if (!condition) {
    throw std::logic_error(message);
  }
}
