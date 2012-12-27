#ifndef SRC_COMMON_CLOCK_H_
#define SRC_COMMON_CLOCK_H_
#include <chrono>
#include <map>
#include <vector>
#include <string>
#include <thread>

// #define SECTION_RECORDING

class Clock {
 public:
  Clock();

  static void setThread();
  static void startSection(const std::string &name);
  static void endSection(const std::string &name);
  static void dumpTimes();

  Clock& start();

  float seconds() const;
  float milliseconds() const;
  size_t microseconds() const;

  typedef std::chrono::high_resolution_clock clock;
  typedef std::chrono::time_point<clock> time_point;

  static time_point now();
  static float secondsSince(const Clock::time_point &then);

 private:
  std::chrono::time_point<clock> start_;

  static std::map<std::string, size_t> sections_;
  static std::vector<std::pair<std::string, Clock>> clocks_;
  static std::string currentSection_;
  static std::thread::id threadID_;
};

// Object that starts a section, and ends when it is destoyed
class ClockSection {
 public:
  explicit ClockSection(const std::string &name);
  ~ClockSection();
 private:
  ClockSection(const ClockSection &);

  const std::string name_;
};

#ifdef SECTION_RECORDING
#define CONCAT_IMPL(x, y) x##y
#define MACRO_CONCAT(x, y) CONCAT_IMPL(x, y)
#define record_section(name) ClockSection MACRO_CONCAT(clock, __COUNTER__)(name)
#else
#define record_section(name)
#endif

#endif  // SRC_COMMON_CLOCK_H_
