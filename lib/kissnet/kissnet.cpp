#include "kissnet.h"
#include <iostream>
#include <cstring> // for strerror
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
#else
#include <WinSock2.h>
#include <ws2tcpip.h>
#endif

namespace kissnet
{
// -----------------------------------------------------------------------------
// Utility Functions
// -----------------------------------------------------------------------------
void init_networking()
{
#ifdef _MSC_VER
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        throw socket_exception("WSAStartup failed\n");
#endif
}
// -----------------------------------------------------------------------------
// Socket Exception
// -----------------------------------------------------------------------------
socket_exception::socket_exception(const std::string& what, bool include_syserr)
    : msg(what)
{
    if (include_syserr)
    {
        msg += ": ";
        msg += strerror(errno);
    }
}

socket_exception::~socket_exception() throw()
{
    // empty
}

const char * socket_exception::what() const throw()
{
    return msg.c_str();
}
// -----------------------------------------------------------------------------

tcp_socket_ptr tcp_socket::create()
{
    tcp_socket_ptr ret(new tcp_socket());
    return ret;
}

tcp_socket_ptr tcp_socket::create(int sockfd)
{
    tcp_socket_ptr ret(new tcp_socket(sockfd));
    return ret;
}

tcp_socket::tcp_socket()
{
    // Create socket
    // TODO move this elsewhere so that it doesn't have to be AF_INET
    sock = socket(AF_INET, SOCK_STREAM, 0);
}

tcp_socket::tcp_socket(int sock_fd)
    : sock(sock_fd)
{
}

tcp_socket::~tcp_socket()
{
    // Make sure we clean up
    close();
}

void tcp_socket::connect(const std::string &addr, const std::string& port)
{
    struct addrinfo *res = NULL, hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(addr.c_str(), port.c_str(), &hints, &res);

    if (::connect(sock, res->ai_addr, res->ai_addrlen) < 0)
        throw socket_exception("Unable to connect", true);

    fillAddr();
}

void tcp_socket::close()
{
    std::cout << "Closing socket!\n";
#ifdef _MSC_VER
    ::closesocket(sock);
#else
    ::close(sock);
#endif
}

void tcp_socket::listen(const std::string &port, int backlog)
{
    // set reuseaddr
    char yes = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0)
    {
        throw socket_exception("Unable to set reuseaddr", true);
    }

    // Fill structs
    struct addrinfo *res, hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (::getaddrinfo(NULL, port.c_str(), &hints, &res) < 0)
        throw socket_exception("Unable to getaddrinfo", false);

    // Bind to local port
    if (::bind(sock, res->ai_addr, res->ai_addrlen) < 0)
        throw socket_exception("Unable to bind", true);

    // Now listen
    if (::listen(sock, backlog) < 0)
        throw socket_exception("Unable to listen", true);
}

tcp_socket_ptr tcp_socket::accept()
{
    int newsock;
    if ((newsock = ::accept(sock, NULL, NULL)) < 0)
        throw socket_exception("Unable to accept", true);

    tcp_socket_ptr ret = create(newsock);
    ret->fillAddr();
    return ret;
}

ssize_t tcp_socket::send(const std::string& data)
{
    ssize_t bytes_sent = 0;
    
    if ((bytes_sent = ::send(sock, data.c_str(), data.size(), 0)) < 0)
        throw socket_exception("Unable to send", true);

    return bytes_sent;
}

ssize_t tcp_socket::recv(char *buffer, size_t buffer_len)
{
    ssize_t bytes_received;

    if ((bytes_received = ::recv(sock, buffer, buffer_len, 0)) < 0)
        throw socket_exception("Unable to recv", true);

    return bytes_received;
}

int tcp_socket::getSocket() const
{
    return sock;
}

std::string tcp_socket::getHostname() const
{
    return ipaddr;
}

std::string tcp_socket::getPort() const
{
    return portStr;
}

bool tcp_socket::operator==(const tcp_socket &rhs) const
{
    return sock == rhs.sock;
}

void tcp_socket::fillAddr()
{
    struct sockaddr_storage addr;
    socklen_t len = sizeof(addr);
    getpeername(sock, (struct sockaddr *) &addr, &len);

    char ipstr[INET6_ADDRSTRLEN];
    int portnum;

    if (addr.ss_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in *)&addr;
        portnum = ntohs(s->sin_port);
        inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
    } else { // AF_INET6
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
        portnum = ntohs(s->sin6_port);
        inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof ipstr);
    }

    ipaddr = ipstr;
    std::stringstream ss; ss << portnum;
    portStr = ss.str();
}

// -----------------------------------------------------------------------------
// socket_set definitions
// -----------------------------------------------------------------------------
socket_set::socket_set()
    : socks()
{
    // Empty
}

socket_set::~socket_set()
{
    // Empty
}

void socket_set::add_socket(tcp_socket *sock)
{
    socks.push_back(sock);
}

void socket_set::remove_socket(tcp_socket *sock)
{
    socks.remove(sock);
}

std::vector<tcp_socket*> socket_set::poll_sockets()
{
    fd_set rset;
    FD_ZERO(&rset);

    int maxfd = -1;
    for (std::list<tcp_socket*>::iterator it = socks.begin();
         it != socks.end(); it++)
    {
        int curfd = (*it)->getSocket();
        FD_SET(curfd, &rset);
        if (curfd > maxfd)
            maxfd = curfd;
    }

    ::select(maxfd + 1, &rset, NULL, NULL, NULL);

    std::vector<tcp_socket*> ret;
    for (std::list<tcp_socket*>::iterator it = socks.begin();
         it != socks.end(); it++)
    {
        int curfd = (*it)->getSocket();
        if (FD_ISSET(curfd, &rset))
            ret.push_back(*it);
    }

    return ret;
}

// End namespace kissnet
};
