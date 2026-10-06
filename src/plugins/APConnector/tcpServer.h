#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <vector>
#include <cstdint>
namespace Plugins::APC { class Ctx; }

class daysTCP {
    public:
explicit daysTCP(Plugins::APC::Ctx& ctx) : ctx(ctx) {}
void acceptConnection();
void pollTCP();
void readTcp();
void closeCli();
void startTcpServer();
void startListen();
void bindSocket();
void send(const std::vector<uint8_t>& data);
void startSocket();
bool startWSA();
private:
    Plugins::APC::Ctx& ctx;

    SOCKET serverSocket = INVALID_SOCKET;
    SOCKET acceptSocket = INVALID_SOCKET;
    WSADATA wsaData;
};
