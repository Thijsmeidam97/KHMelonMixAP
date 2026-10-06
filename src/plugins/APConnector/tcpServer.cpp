#include <WinSock2.h>
#include <WS2tcpip.h>
#include "tcpServer.h"
#include <vector>

static std::vector<uint8_t> rx;
#define BASEBYTELENGTH 4
bool startWSA() {
    return (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0);
}

void startSocket()
{
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (serverSocket != INVALID_SOCKET) {
        u_long mode = 1;

        if (ioctlsocket(serverSocket, FIONBIO, &mode) == SOCKET_ERROR) {
            closesocket(serverSocket);
            serverSocket = INVALID_SOCKET;
        }
    }
}

void bindSocket() {
    if (serverSocket != INVALID_SOCKET) {
        sockaddr_in address{};
        address.sin_family = AF_INET;
        inet_pton(AF_INET, "127.0.0.1",&address.sin_addr);
        address.sin_port = htons(43050);

        if (bind(serverSocket, (SOCKADDR*)&address, sizeof(address)) == SOCKET_ERROR) {
            closesocket(serverSocket);
            WSACleanup();
            serverSocket = INVALID_SOCKET;
        }
    }
}

void startListen()
{
    if (serverSocket != INVALID_SOCKET) {
        if (listen(serverSocket, 1) == SOCKET_ERROR) {
            closesocket(serverSocket);
            serverSocket = INVALID_SOCKET;
        }
    }
}

void startTcpServer()
{
    if (startWSA()) {
        startSocket();
        bindSocket();
        startListen();
    }
}

void closeCli()
{
    closesocket(acceptSocket);
    acceptSocket = INVALID_SOCKET;
    rx.clear();
}

void readTcp()
{
    if (acceptSocket != INVALID_SOCKET) {
        uint8_t buffer[4096];
        int bufferLength = recv(acceptSocket, reinterpret_cast<char*>(buffer), sizeof(buffer), 0);

        if (bufferLength > 0) {
            rx.insert(rx.end(), buffer, buffer + bufferLength);
            bool processing = true;

            while (processing && rx.size() >= BASEBYTELENGTH) {
                uint32_t size = 0;

                for (unsigned i = 0; i < BASEBYTELENGTH; ++i)
                    size |= uint32_t(rx[i]) << (i * 8);

                if (size == 0 || size > 65536) {
                    closeCli();
                    processing = false;
                }
                else if (rx.size() >= size + BASEBYTELENGTH) {
                    std::vector<uint8_t> body(rx.begin() + BASEBYTELENGTH,
                                              rx.begin() + BASEBYTELENGTH + size);
                    rx.erase(rx.begin(), rx.begin() + BASEBYTELENGTH + size);
                    // readFrame(body);
                    processing = acceptSocket != INVALID_SOCKET;
                }
                else
                    processing = false;
            }
        }
        else if (bufferLength == 0)
            closeCli();
        else if (WSAGetLastError() != WSAEWOULDBLOCK)
            closeCli();
    }
}

void acceptConnection()
{
    if ((acceptSocket == INVALID_SOCKET) && (serverSocket != INVALID_SOCKET)) {
        acceptSocket = accept(serverSocket, nullptr, nullptr);

        if (acceptSocket != INVALID_SOCKET) {
            u_long mode = 1;
            if (ioctlsocket(acceptSocket, FIONBIO, &mode) == SOCKET_ERROR) {
                closesocket(acceptSocket);
                acceptSocket = INVALID_SOCKET;
            }
        }
        else if (WSAGetLastError() != WSAEWOULDBLOCK) {
            closesocket(serverSocket);
            serverSocket = INVALID_SOCKET;
        }
    }
}

void pollTCP(){
    startTcpServer();
    acceptConnection();
    readTcp();
}