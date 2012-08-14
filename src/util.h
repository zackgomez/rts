#pragma once
#include <string>
#include "Exception.h"

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
