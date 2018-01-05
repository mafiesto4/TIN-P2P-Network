#pragma once

#include "Node.h"

// The main peer-to-peer server controller. Exposes various functionalities and manages the network connections and structure.
class Service
{
public:

	// The global instance of the service (singleton)
	static Service Instance;

private:

	Node* _local = nullptr;
	int _socket = -1;

public:

	// Gets the local server node
	Node* GetLocalNode() const
	{
		return _local;
	}

	// Returns true if serivce is running now, otherwise false
	bool IsRunning() const
	{
		return _local != nullptr;
	}

public:

	// Starts the service at the given port (uses the local address)
	void Start(ushort port = 0);

	// Stops the running service
	void Stop();
};
