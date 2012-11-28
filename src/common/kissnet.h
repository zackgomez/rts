#ifndef SRC_COMMON_KISSNET_H_
#define SRC_COMMON_KISSNET_H_

#include <string>
#include <exception>
#include <vector>
#include <list>
#include <memory>

namespace kissnet {

void init_networking();
class socket_exception : public std::exception {
 public:
  socket_exception(const std::string& what, bool include_syserr = false);

  ~socket_exception() throw();

  const char * what() const throw();
  int getErrno() const throw();

 private:
  std::string msg_;
  int errno_;
};

class tcp_socket;
typedef std::shared_ptr<tcp_socket> tcp_socket_ptr;

class tcp_socket {
 public:
  static tcp_socket_ptr create();
  static tcp_socket_ptr create(int sockfd);

  ~tcp_socket();

  void bind(const std::string &port);

  void connect(const std::string& addr, const std::string& port);
  void nonblockingConnect(const std::string& addr, const std::string& port);
  // returns this
  tcp_socket* setBlocking();
  tcp_socket* setNonBlocking();
  void close();

  void listen(const std::string& port, int backlog);
  tcp_socket_ptr accept();

  int send(const std::string& data);
  int recv(char* buffer, size_t buffer_len);

  int getError() const;
  std::string getHostname() const;
  std::string getPort() const;
  std::string getLocalPort() const;
  std::string getLocalAddr() const;

  bool operator==(const tcp_socket& rhs) const;

  // Bare metal functions
  int  getSocket() const;
  // TODO(zack) add bool connected() const;

 private:
  tcp_socket();
  explicit tcp_socket(int sockfd);

  void fillAddr() const;
  void setReuseAddr();

  int sock;

  mutable std::string ipaddr;
  mutable std::string portStr;
};

class socket_set {
 public:
  socket_set();
  ~socket_set();

  void add_read_socket(tcp_socket_ptr sock);
  void add_write_socket(tcp_socket_ptr sock);
  void remove_socket(tcp_socket_ptr sock);

  // timeout is filled with remaining time
  std::vector<tcp_socket_ptr> poll_sockets(double &timeout);
 private:
  std::list<tcp_socket_ptr> rsocks_, wsocks_;
  int maxfd_;
};
};  // kissnet

#endif  // SRC_COMMON_KISSNET_H_
