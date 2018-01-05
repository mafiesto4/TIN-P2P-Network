#pragma once

#include "Config.h"

// Base for all network messages
struct NetworkMsg
{
	byte Type;
};

// Maximum size (in bytes) for a single network message
#define MAX_NETWORK_MSG_SIZE 386

// Message types
#define MSG_TYPE_NETOWRK_CHANGE 1

// Message send after network structure changed (host added/removed)
struct NetworkChangeMsg : NetworkMsg
{
	byte IsNew;
	byte NameLength;
	char Name[MAX_MSG_NAME_LENGTH];
};
static_assert(sizeof(NetworkChangeMsg) <= MAX_NETWORK_MSG_SIZE, "Invalid message size");
