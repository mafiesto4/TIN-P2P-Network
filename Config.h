#pragma once

#if _WIN32

#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#endif

#include <stdio.h>

typedef unsigned short ushort;
typedef unsigned int uint;
