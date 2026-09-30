#include "State.h"
#include "../../NDS.h"
#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
#include <chrono>
#include <thread>
namespace Plugins::APC {
namespace {
#ifdef _WIN32
int lastErr() {
    return WSAGetLastError();
}
bool would(int err) {
    return err == WSAEWOULDBLOCK || err == WSAETIMEDOUT;
}
void closeSock(APCSock sock) {
    if (sock != APCBadSock) closesocket(sock);
}
bool setNon(APCSock sock) {
    u_long mode = 1;
    return ioctlsocket(sock, FIONBIO, &mode) == 0;
}
#else
int lastErr() {
    return errno;
}
bool would(int err) {
    return err == EWOULDBLOCK || err == EAGAIN;
}
void closeSock(APCSock sock) {
    if (sock != APCBadSock) ::close(sock);
}
bool setNon(APCSock sock) {
    const int flags = fcntl(sock, F_GETFL, 0);
    return flags >= 0 && fcntl(sock, F_SETFL, flags | O_NONBLOCK) == 0;
}
#endif
}
Ctx::Ctx(MsgCb fn) : cb(std::move(fn)) {
#ifdef _WIN32
    WSADATA data{};
    ws = WSAStartup(MAKEWORD(2, 2), &data) == 0;
#else
    ws = true;
#endif
}
Ctx::~Ctx() {
    closeCon();
    closeSrv();
#ifdef _WIN32
    if (ws) WSACleanup();
#endif
}
void Ctx::setOn(bool value) {
    on = value;
    if (!on) {
        closeCon();
        closeSrv();
    }
}
void Ctx::closeCon() {
    closeSock(cli);
    cli = APCBadSock;
    rx.clear();
    lock = false;
}
void Ctx::closeSrv() {
    closeSock(srv);
    srv = APCBadSock;
}
bool Ctx::send(const std::string& text) {
    bool ok = cli != APCBadSock;
    size_t off = 0;
    const auto end = Clock::now() + std::chrono::seconds(2);
    while (ok && off < text.size()) {
#ifdef _WIN32
        const int count = ::send(cli, text.data() + off, (int)(text.size() - off), 0);
#else
        const ssize_t count = ::send(cli, text.data() + off, text.size() - off, MSG_NOSIGNAL);
#endif
        if (count > 0)
            off += (size_t)count;
        else if (count == 0 || !would(lastErr()) || Clock::now() >= end) {
            closeCon();
            ok = false;
        }
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return ok;
}
bool Ctx::wsOk() const {
    return ws;
}
void Ctx::start() {
    bool run = srv == APCBadSock && wsOk();
    for (int port = 43055; run && port <= 43060; ++port) {
        APCSock sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock != APCBadSock) {
            int reuse = 1;
#ifdef _WIN32
            setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));
#else
            setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
#endif
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons((uint16_t)port);
            inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
            if (::bind(sock, (const sockaddr*)&addr, sizeof(addr)) == 0 && ::listen(sock, 1) == 0 && setNon(sock)) {
                srv = sock;
                run = false;
            }
            else
                closeSock(sock);
        }
    }
}
void Ctx::accept() {
    if (srv != APCBadSock && cli == APCBadSock) {
        sockaddr_in addr{};
#ifdef _WIN32
        int len = sizeof(addr);
#else
        socklen_t len = sizeof(addr);
#endif
        APCSock sock = ::accept(srv, (sockaddr*)&addr, &len);
        if (sock == APCBadSock) {
            if (!would(lastErr())) closeSrv();
        }
        else if (!setNon(sock))
            closeSock(sock);
        else {
            cli = sock;
            rx.clear();
            last = Clock::now();
        }
    }
}
void Ctx::showMsg() {
    const auto now = Clock::now();
    if (!msgs.empty() && cb && std::chrono::duration<double>(now - sent).count() >= gap) {
        cb(msgs.front());
        msgs.pop_front();
        sent = now;
    }
}
void Ctx::readCli() {
    char buf[8192];
    bool run = cli != APCBadSock;
    while (run) {
#ifdef _WIN32
        const int count = ::recv(cli, buf, sizeof(buf), 0);
#else
        const ssize_t count = ::recv(cli, buf, sizeof(buf), 0);
#endif
        if (count > 0) {
            last = Clock::now();
            rx.append(buf, (size_t)count);
            size_t at = 0;
            while ((at = rx.find('\n')) != std::string::npos) {
                std::string line = rx.substr(0, at);
                rx.erase(0, at + 1);
                if (!line.empty() && line.back() == '\r') line.pop_back();
                if (!line.empty()) procLine(line);
                if (cli == APCBadSock) run = false;
            }
        }
        else {
            if (count == 0 || !would(lastErr())) closeCon();
            run = false;
        }
    }
}
void Ctx::wait() {
    const auto end = Clock::now() + std::chrono::seconds(2);
    while (lock && cli != APCBadSock && Clock::now() < end) {
        readCli();
        if (lock) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    lock = false;
}
void Ctx::net() {
    start();
    accept();
    if (cli != APCBadSock) {
        readCli();
        if (lock) wait();
        if (cli != APCBadSock && std::chrono::duration<double>(Clock::now() - last).count() > 5.0) closeCon();
    }
}
void Ctx::clear() {
    item = {};
    chr = {};
    pre = {};
    sig = {};
    revs.clear();
    sig.base = -1;
    open = {};
    gate = {};
    holo = {};
    day = {};
    rx.clear();
    msgs.clear();
    frm = 0;
    lock = false;
    last = Clock::now();
    sent = last;
}
void Ctx::loadRom() {
    clear();
    if (on) start();
}
void Ctx::loadSt() {
    clear();
}
void Ctx::poll() {
    if (on && nds) {
        net();
        run();
    }
}
}
