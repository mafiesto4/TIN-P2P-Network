#pragma once

#if _WIN32

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <limits.h>

#endif

#include <stdio.h>
#include <assert.h>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>

typedef unsigned char byte;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

typedef std::lock_guard<std::mutex> scope_lock;

#undef min
#undef max

#define MAX_MSG_NAME_LENGTH 254
