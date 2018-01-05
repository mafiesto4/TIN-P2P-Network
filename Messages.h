#pragma once

#include "Config.h"

// Base for all network messages
struct NetworkMsg
{
	byte Type;
};

// Message send after network structure changed (host added/removed)
struct NetworkChangeMsg : NetworkMsg
{
	byte IsNew;
	byte NameLength;
	char Name[MAX_MSG_NAME_LENGTH];
};
