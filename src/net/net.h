#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define WSAEWOULDBLOCK		EWOULDBLOCK
#define WSAECONNREFUSED     ECONNREFUSED
#define WSAECONNABORTED		ECONNABORTED
#define WSAECONNRESET		ECONNRESET

#define ioctlsocket ioctl
#define closesocket close

#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#endif