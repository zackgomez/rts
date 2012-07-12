#pragma once
#include <string>
#include <exception>
#include <vector>
#include <list>
#include <boost/shared_ptr.hpp>

namespace kissnet
{

void init_networking();
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
class socket_exception
    : public std::exception
{
public:
    socket_exception(const std::string& what, bool include_syserr = false);

    ~socket_exception() throw();

    const char * what() const throw();

private:
    std::string msg;
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
class tcp_socket;
typedef boost::shared_ptr<tcp_socket> tcp_socket_ptr;

class tcp_socket
{
public:
    static tcp_socket_ptr create();
    static tcp_socket_ptr create(int sockfd);

    ~tcp_socket();

    void connect(const std::string& addr, const std::string& port);
    void close();

    void listen(const std::string& port, int backlog);
    tcp_socket_ptr accept();

    ssize_t send(const std::string& data);
    ssize_t recv(char* buffer, int buffer_len);

    std::string getHostname() const;
    std::string getPort() const;

    bool operator==(const tcp_socket& rhs) const;

    // Bare metal functions
    int  getSocket() const;

private:
    tcp_socket();
    tcp_socket(int sockfd);

    void fillAddr();

    int sock;

    std::string ipaddr;
    std::string portStr;
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

class socket_set
{
public:
    socket_set();
    ~socket_set();

    void add_socket(tcp_socket* sock);
    void remove_socket(tcp_socket* sock);

    std::vector<tcp_socket*> poll_sockets();
private:
    std::list<tcp_socket*> socks;
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
};
