#include <WinSock2.h>
#include <WS2tcpip.h>

SOCKET serverSocket = INVALID_SOCKET;
SOCKET acceptSocket = INVALID_SOCKET;
WSADATA wsaData;