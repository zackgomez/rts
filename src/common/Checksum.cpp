#include "common/Checksum.h"
#include <cstdio>
#ifdef _MSC_VER
#define snprintf _snprintf
#endif

Checksum::Checksum() {
}

Checksum::~Checksum() {
}

std::string Checksum::checksumToString(checksum_t checksum) {
  // 2 characters per byte + null character
  const size_t kChecksumStrLength = 2 * sizeof(checksum) + 1;
  char buf[kChecksumStrLength];
  snprintf(buf, sizeof(buf), "%X", checksum);
  return std::string(buf);
}

checksum_t Checksum::getChecksum() const {
  return checksum_.checksum();
}

Checksum& Checksum::process(const void *data, size_t size) {
  checksum_.process_bytes(data, size);
  return *this;
}

Checksum& Checksum::process(std::istream &is) {
  const size_t kFileBufSize = 1024;
  char buf[kFileBufSize];
  boost::crc_32_type checksum;
  while (is.read(buf, kFileBufSize)) {
    process(buf, is.gcount());
  }
  return *this;
}
