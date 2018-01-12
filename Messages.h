#pragma once

#include "Config.h"

// Base for all network messages
struct NetworkMsg
{
	byte Type;
	ushort Port;
};

// Maximum size (in bytes) for a single network message
#define MAX_NETWORK_MSG_SIZE 386

// Message types
#define MSG_TYPE_NETOWRK_CHANGE 1
#define MSG_TYPE_TRANSFER_INIT 7

// Message send after network structure changed (host added/removed)
struct NetworkChangeMsg : NetworkMsg
{
	byte IsNew;
	byte NameLength;
	char Name[MAX_MSG_NAME_LENGTH];
};
static_assert(sizeof(NetworkChangeMsg) <= MAX_NETWORK_MSG_SIZE, "Invalid message size");

// Message send on file data transfer initialization (send by the node requesting data)
struct NetworkTransferInitMsg : NetworkMsg
{
	ushort TcpPort;
	Hash Hash;
};
static_assert(sizeof(NetworkTransferInitMsg) <= MAX_NETWORK_MSG_SIZE, "Invalid message size");
