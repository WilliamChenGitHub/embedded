#include "Socket.h"


#if defined(__unix__) || defined(__linux__)
#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <sys/epoll.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>


#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>


ATTRIBUTE_WEAK SOCKET_T SocketCreate(SOCKET_DOMAIN_ET domain, SOCKET_TYPE_ET type, SOCKET_PROTOCAL_ET protocal)
{
    int32_t skt = socket(AF_INET, SOCK_STREAM, 0);

    return (SOCKET_T)skt;
}


#elif defined(_WIN32)

#elif defined(__APPLE__)


#else


#endif

