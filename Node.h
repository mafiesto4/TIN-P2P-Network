#pragma once

#include "Config.h"

// Represents a single pee-to-2peer client node connected to the network
class Node
{
private:

	sockaddr_in _address;

public:

	Node(sockaddr_in& addr)
	{
		_address = addr;
	}

	// Returns true if this node is a local node
	bool IsLocal() const;
};
