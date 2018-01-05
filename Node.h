#pragma once

#include "Config.h"
#include <algorithm>

// Represents a single pee-to-2peer client node connected to the network
class Node
{
private:

	sockaddr_in _address;
	char Name[MAX_MSG_NAME_LENGTH + 1];

public:

	Node(sockaddr_in& addr, const char* name)
	{
		_address = addr;

		int nameLength = std::min((int)strlen(name), MAX_MSG_NAME_LENGTH);
		memcpy(Name, name, nameLength);
		Name[nameLength] = 0;
	}

public:

	// Gets the name of the node (used for the user-interface and basic identification)
	char* GetName()
	{
		return Name;
	}

	// Returns true if this node is a local node
	bool IsLocal() const;
};
