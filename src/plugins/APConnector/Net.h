#pragma once
#ifdef _WIN32
#include <winsock2.h>
using APCSock = SOCKET;
constexpr APCSock APCBadSock = INVALID_SOCKET;
#else
using APCSock = int;
constexpr APCSock APCBadSock = -1;
#endif
