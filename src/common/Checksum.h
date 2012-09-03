#ifndef SRC_COMMON_CHECKSUM_H_
#define SRC_COMMON_CHECKSUM_H_
#include <boost/crc.hpp>
#include <istream>

typedef uint32_t checksum_t;

class Checksum {
 public:
  Checksum();
  ~Checksum();

  static std::string checksumToString(checksum_t checksum);

  checksum_t getChecksum() const;

  Checksum& process(const void *data, size_t size);
  Checksum& process(std::istream &is);

 private:
  boost::crc_32_type checksum_;
};

#endif  // SRC_COMMON_CHECKSUM_H_
