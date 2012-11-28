#include "common/kissnet.h"
#include <iostream>
#include <cstring>  // for strerror
#include <cstdlib>
#include <errno.h>
#include <sstream>

#ifndef _MSC_VER
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#else
#include <WinSock2.h>
#include <ws2tcpip.h>
#endif

#include "common/util.h"
#include "common/Logger.h"

namespace kissnet {
// -----------------------------------------------------------------------------
// Utility Functions
// -----------------------------------------------------------------------------
void init_networking() {
#ifdef _MSC_VER
#pragma comment(lib, "Ws2_32.lib")
  // Initialize Winsock
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    throw socket_exception("WSAStartup failed\n");
#endif
}

fd_set getFDSet(const std::list<kissnet::tcp_socket_ptr> &socks) {
  fd_set ret;
  FD_ZERO(&ret);
  for (auto it = socks.begin(); it != socks.end(); it++) {
    int curfd = (*it)->getSocket();
    FD_SET(curfd, &ret);
  }
  return ret;
}

// -----------------------------------------------------------------------------
// Socket Exception
// -----------------------------------------------------------------------------
socket_exception::socket_exception(const std::string& what, bool include_syserr)
  : msg_(what),
#ifndef _MSC_VER
    errno_(errno) {
#else
    errno_(WSAGetLastError()) {
#endif
  if (include_syserr) {
    msg_ += ": ";
    msg_ += strerror(errno);
  }
}

socket_exception::~socket_exception() throw() {
  // empty
}

const char * socket_exception::what() const throw() {
  return msg_.c_str();
}

int socket_exception::getErrno() const throw() {
  return errno_;
}
// -----------------------------------------------------------------------------

tcp_socket_ptr tcp_socket::create() {
  tcp_socket_ptr ret(new tcp_socket());
  return ret;
}

tcp_socket_ptr tcp_socket::create(int sockfd) {
  tcp_socket_ptr ret(new tcp_socket(sockfd));
  return ret;
}

tcp_socket::tcp_socket() {
  // Create socket
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    throw new socket_exception("socket:", true);
  }
  setReuseAddr();
}

tcp_socket::tcp_socket(int sock_fd)
  : sock(sock_fd) {
}

tcp_socket::~tcp_socket() {
  // Make sure we clean up
  close();
}

void tcp_socket::bind(const std::string &port) {
  struct addrinfo *res = nullptr, hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if (getaddrinfo(nullptr, port.c_str(), &hints, &res) < 0) {
    throw socket_exception("getaddrinfo");
  }

  // Bind to local port
  if (::bind(sock, res->ai_addr, res->ai_addrlen) < 0) {
    throw socket_exception("Unable to bind", true);
  }
}

// TODO(zack): replace with general purpose setNonblocking on socket
void tcp_socket::nonblockingConnect(
    const std::string& addr,
    const std::string& port) {
#ifdef _MSC_VER
  unsigned long val = 1;
  ioctlsocket(sock, FIONBIO, &val);
#else
  fcntl(sock, F_SETFL, O_NONBLOCK);
#endif


  try {
    connect(addr, port);
  } catch (socket_exception &e) {
#ifdef _MSC_VER
    if (e.getErrno() != WSAEINPROGRESS && e.getErrno() != WSAEWOULDBLOCK) {
#else
    if (e.getErrno() != EINPROGRESS) {
#endif
      LOG(WARNING) << "errno: " << e.getErrno() << '\n';
      throw e;
    }
  }
}

int tcp_socket::getError() const {
  int socket_err = 0;
  socklen_t optsize = sizeof(socket_err);
  int ret =
      getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&socket_err, &optsize);
  if (ret == -1) {
    throw socket_exception("unable to getsockopt", true);
  }
  return socket_err;
}

tcp_socket* tcp_socket::setBlocking() {
#ifdef _MSC_VER
  unsigned long val = 0;
  ioctlsocket(sock, FIONBIO, &val);
#else
  fcntl(sock, F_SETFL, 0);
#endif
  return this;
}

tcp_socket* tcp_socket::setNonBlocking() {
#ifdef _MSC_VER
  unsigned long val = 1;
  ioctlsocket(sock, FIONBIO, &val);
#else
  fcntl(sock, F_SETFL, O_NONBLOCK);
#endif
  return this;
}

void tcp_socket::connect(const std::string &addr, const std::string& port) {
  struct addrinfo *res = nullptr, hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if (getaddrinfo(addr.c_str(), port.c_str(), &hints, &res)) {
    throw socket_exception("unable to resolve address", true);
  }

  int ret;
  if ((ret = ::connect(sock, res->ai_addr, res->ai_addrlen)) < 0) {
    LOG(ERROR) << "ret: " << ret << '\n';
    throw socket_exception("Unable to connect", true);
  }
}

void tcp_socket::close() {
#ifdef _MSC_VER
  ::closesocket(sock);
#else
  ::close(sock);
#endif
}

void tcp_socket::setReuseAddr() {
#ifdef _MSC_VER
  char yes = 1;
#else
  int yes = 1;
#endif
  if (::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
    throw socket_exception("Unable to set reuseaddr", true);
  }
}

void tcp_socket::listen(const std::string &port, int backlog) {
  // Fill structs
  struct addrinfo *res, hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if (::getaddrinfo(nullptr, port.c_str(), &hints, &res) < 0) {
    throw socket_exception("Unable to getaddrinfo", false);
  }

  // Bind to local port
  if (::bind(sock, res->ai_addr, res->ai_addrlen) < 0) {
    throw socket_exception("Unable to bind", true);
  }

  // Now listen
  if (::listen(sock, backlog) < 0) {
    throw socket_exception("Unable to listen", true);
  }
}

tcp_socket_ptr tcp_socket::accept() {
  int newsock;
  if ((newsock = ::accept(sock, nullptr, nullptr)) < 0)
    throw socket_exception("Unable to accept", true);

  tcp_socket_ptr ret = create(newsock);
  return ret;
}

int tcp_socket::send(const std::string& data) {
  int bytes_sent = 0;

  if ((bytes_sent = ::send(sock, data.c_str(), data.size(), 0)) < 0) {
    throw socket_exception("Unable to send", true);
  }

  return bytes_sent;
}

int tcp_socket::recv(char *buffer, size_t buffer_len) {
  int bytes_received;

  // TODO(zack): this is a SERIOUS HACK, should fix this...
  // Deal with being interrupted properly
  // Also think about what to do when these return 0?  Should we close
  // the socket?
  while ((bytes_received = ::recv(sock, buffer, buffer_len, 0)) < 0) {
    if (errno != EINTR) {
      throw socket_exception("Unable to recv", true);
    } else if (errno == EINTR) {
      LOG(WARNING) << "Interrupted during recv call\n";
    }
  }

  return bytes_received;
}

int tcp_socket::getSocket() const {
  return sock;
}

std::string tcp_socket::getHostname() const {
  if (ipaddr.empty()) {
    fillAddr();
  }
  return ipaddr;
}

std::string tcp_socket::getPort() const {
  if (portStr.empty()) {
    fillAddr();
  }
  return portStr;
}

std::string tcp_socket::getLocalPort() const {
  struct sockaddr_storage addr;
  socklen_t len = sizeof(addr);
  int ret = getsockname(sock, (struct sockaddr*)&addr, &len);
  invariant(!ret, "unacceptable error getting local port");
  std::stringstream ss;
  ss << ntohs(((struct sockaddr_in*) &addr)->sin_port);
  return ss.str();
}

std::string tcp_socket::getLocalAddr() const {
  struct sockaddr_storage addr;
  socklen_t len = sizeof(addr);
  int ret = getsockname(sock, (struct sockaddr*)&addr, &len);
  invariant(!ret, "unacceptable error getting local port");

  char ipstr[INET6_ADDRSTRLEN];
  if (addr.ss_family == AF_INET) {
    struct sockaddr_in *s = (struct sockaddr_in *)&addr;
    inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
  } else {  // AF_INET6
    struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
    inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof ipstr);
  }
  return ipstr;
}

bool tcp_socket::operator==(const tcp_socket &rhs) const {
  return sock == rhs.sock;
}

void tcp_socket::fillAddr() const {
  struct sockaddr_storage addr;
  socklen_t len = sizeof(addr);
  getpeername(sock, (struct sockaddr *) &addr, &len);

  char ipstr[INET6_ADDRSTRLEN];
  int portnum;

  if (addr.ss_family == AF_INET) {
    struct sockaddr_in *s = (struct sockaddr_in *)&addr;
    portnum = ntohs(s->sin_port);
    inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
  } else {  // AF_INET6
    struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
    portnum = ntohs(s->sin6_port);
    inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof ipstr);
  }

  ipaddr = ipstr;
  std::stringstream ss;
  ss << portnum;
  portStr = ss.str();
}

// -----------------------------------------------------------------------------
// socket_set definitions
// -----------------------------------------------------------------------------
socket_set::socket_set() : maxfd_(-1) {
  // Empty
}

socket_set::~socket_set() {
  // Empty
}

void socket_set::add_read_socket(tcp_socket_ptr sock) {
  rsocks_.push_back(sock);
  maxfd_ = std::max(maxfd_, sock->getSocket());
}

void socket_set::add_write_socket(tcp_socket_ptr sock) {
  wsocks_.push_back(sock);
  maxfd_ = std::max(maxfd_, sock->getSocket());
}

void socket_set::remove_socket(tcp_socket_ptr sock) {
  rsocks_.remove(sock);
  wsocks_.remove(sock);
}

std::vector<tcp_socket_ptr> socket_set::poll_sockets(double &timeout) {
  invariant(timeout >= 0.f, "timeout must be positive");

  double secs;
  double partial_secs = modf(timeout, &secs);
  struct timeval tv;
  tv.tv_sec = (int) secs;
  tv.tv_usec = 1e6 * partial_secs;

  fd_set rset = getFDSet(rsocks_);
  fd_set wset = getFDSet(wsocks_);

  std::vector<tcp_socket_ptr> ret;

  int select_ret = ::select(maxfd_ + 1, &rset, &wset, nullptr, &tv);
  if (select_ret < 0) {
    throw socket_exception("error in select", true);
  } else if (select_ret == 0) {
    timeout = 0.f;
    return ret;
  }
  // select_ret > 0

  for (auto it = rsocks_.begin(); it != rsocks_.end(); it++) {
    int curfd = (*it)->getSocket();
    if (FD_ISSET(curfd, &rset)) {
      ret.push_back(*it);
    }
  }
  // TODO(zack): return these in a separate list
  for (auto it = wsocks_.begin(); it != wsocks_.end(); it++) {
    int curfd = (*it)->getSocket();
    if (FD_ISSET(curfd, &wset)) {
      ret.push_back(*it);
    }
  }

  // Update timeout
  timeout = tv.tv_sec + tv.tv_usec / 1e6;

  return ret;
}
};  // kissnet
