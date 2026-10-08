#include <WinSock2.h>
#include <WS2tcpip.h>
#include "tcpServer.h"
#include <vector>
#include "State.h"

static std::vector<uint8_t> rx;
#define BASEBYTELENGTH 4

bool daysTCP::startWSA() {
    return (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0);
}

void daysTCP::startSocket()
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

void daysTCP::bindSocket() {
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

void daysTCP::startListen()
{
    if (serverSocket != INVALID_SOCKET) {
        if (listen(serverSocket, 1) == SOCKET_ERROR) {
            closesocket(serverSocket);
            serverSocket = INVALID_SOCKET;
        }
    }
}

void daysTCP::startTcpServer()
{
    if (serverSocket == INVALID_SOCKET) {
        if (startWSA()) {
            startSocket();
            bindSocket();
            startListen();
        }
    }
}
void daysTCP::closeCli()
{
    closesocket(acceptSocket);
    acceptSocket = INVALID_SOCKET;
    rx.clear();
}

void daysTCP::readTcp()
{
    if (acceptSocket != INVALID_SOCKET) {
        uint8_t buffer[4096];
        int length = recv(acceptSocket, reinterpret_cast<char*>(buffer), sizeof(buffer), 0);

        if (length > 0) {
            rx.insert(rx.end(), buffer, buffer + length);

            while (rx.size() >= 2) {
                uint8_t opcode = rx[0];
                uint8_t size = rx[1];
                size_t total = size + 2;

                if (rx.size() >= total) {
                    ctx.commandHandling(opcode, size, rx.data());
                    rx.erase(rx.begin(), rx.begin() + total);
                }
                else {
                    break;
                }
            }
        }
        else if (length == 0)
            closeCli();
        else if (WSAGetLastError() != WSAEWOULDBLOCK)
            closeCli();
    }
}

void daysTCP::send(const std::vector<uint8_t>& data)
{
    if (acceptSocket != INVALID_SOCKET) {
        uint32_t size = data.size();
        std::vector<uint8_t> packet;

        packet.insert(packet.end(), data.begin(), data.end());
        ::send(acceptSocket, reinterpret_cast<const char*>(packet.data()),
               packet.size(), 0);
    }
}

void daysTCP::acceptConnection()
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
void daysTCP::pollTCP(){
    acceptConnection();
    readTcp();
}
