#include "common/Clock.h"
#include "common/util.h"

using std::chrono::duration_cast;

std::map<std::string, size_t> Clock::sections_;
std::vector<std::pair<std::string, Clock>> Clock::clocks_;
std::string Clock::currentSection_;
std::thread::id Clock::threadID_;

Clock::Clock() {
}

void Clock::setThread() {
  threadID_ = std::this_thread::get_id();
}

void Clock::startSection(const std::string &name) {
#ifdef SECTION_RECORDING
  if (std::this_thread::get_id() != threadID_) {
    return;
  }
  if (!currentSection_.empty()) {
    currentSection_ += '.';
  }
  currentSection_ += name;
  clocks_.push_back(std::pair<std::string, Clock>(name, Clock().start()));
#endif
}

void Clock::endSection(const std::string &name) {
#ifdef SECTION_RECORDING
  if (std::this_thread::get_id() != threadID_) {
    return;
  }
  invariant(!clocks_.empty(), "can't end empty section!");
  if (clocks_.back().first != name) {
    std::cout << "MISMATCH: " << clocks_.back().first << " vs " << name << '\n';
  }
  invariant(clocks_.back().first == name, "mismatched start/end section");

  size_t t = clocks_.back().second.microseconds();
  sections_[name] += t;
  // don't double count top level sections
  if (name != currentSection_) {
    sections_[currentSection_] += t;
  }

  clocks_.pop_back();
  currentSection_.clear();
  for (auto &pair : clocks_) {
    if (!currentSection_.empty()) {
      currentSection_ += '.';
    }
    currentSection_ += pair.first;
  }
#endif
}

void Clock::dumpTimes() {
#ifdef SECTION_RECORDING
  invariant(clocks_.empty(), "unfinished sections!");

  std::cout << "Times:\n";
  for (auto &pair : sections_) {
    std::cout << pair.first << " : " << pair.second / 1e3 << "ms\n";
  }
  std::cout << "End Times.\n";

  sections_.clear();
  clocks_.clear();
  currentSection_.clear();
#endif
}

Clock::time_point Clock::now() {
  return clock::now();
}

float Clock::secondsSince(const Clock::time_point &then) {
  auto end = Clock::now();
  auto usec = duration_cast<std::chrono::microseconds>(end - then).count();
  return usec / 1e6f;
}

Clock& Clock::start() {
  start_ = Clock::now();
  return *this;
}

size_t Clock::microseconds() const {
  auto end = clock::now();
  return duration_cast<std::chrono::microseconds>(end - start_).count();
}

float Clock::milliseconds() const {
  return microseconds() / 1000.f;
}

float Clock::seconds() const {
  return microseconds() / 1e6;
}

ClockSection::ClockSection(const std::string &name)
  : name_(name) {
  Clock::startSection(name_);
}

ClockSection::~ClockSection() {
  Clock::endSection(name_);
}
