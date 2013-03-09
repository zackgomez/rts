#ifndef SRC_COMMON_CHECKSUM_H_
#define SRC_COMMON_CHECKSUM_H_
#include <boost/crc.hpp>
#include <istream>
#include <json/json.h>

typedef uint32_t checksum_t;

class Checksum {
 public:
  Checksum();
  ~Checksum();

  static std::string checksumToString(checksum_t checksum);

  checksum_t getChecksum() const;

  template<typename T>
  Checksum& process(const T& val);
  Checksum& process(const std::string &val);
  Checksum& process(const Json::Value &val);
  //Checksum& process(const Json::Value &val);
  Checksum& process(std::istream &is);
  Checksum& process(const void *data, size_t size);

 private:
  boost::crc_32_type checksum_;
};

template<typename T>
Checksum& Checksum::process(const T &val) {
  return process(&val, sizeof(val));
}

#endif  // SRC_COMMON_CHECKSUM_H_
