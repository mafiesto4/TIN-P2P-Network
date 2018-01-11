#pragma once

#if _WIN32

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <Ws2tcpip.h>
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

// Computed file hash data storage
struct Hash
{
	char Data[16];
};

// Compares two file hashes and returns true if both are equal
inline bool operator==(const Hash& a, const Hash& b)
{
	return memcmp(a.Data, b.Data, sizeof(Hash)) == 0;
}

// Compares two file addresses and returns true if both are equal
inline bool operator==(const sockaddr_in& a, const sockaddr_in& b)
{
	return memcmp(&a, &b, sizeof(sockaddr_in)) == 0;
}

typedef std::lock_guard<std::recursive_mutex> scope_lock;

#undef min
#undef max

#define MAX_PORT_NUM 65500
#define MAX_MSG_NAME_LENGTH 254
#define MAX_ACTIVE_TRANSFERS_COUNT 12

#define DEFAULT_MSG_PORT 2600
