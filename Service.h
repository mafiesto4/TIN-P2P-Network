#pragma once

#include "Node.h"
#include "Socket.h"

// The main peer-to-peer server controller. Exposes various functionalities and manages the network connections and structure.
class Service
{
public:

	// The global instance of the service (singleton)
	static Service Instance;

private:

	Node* _local = nullptr;
	std::vector<Node*> _nodes;
	std::recursive_mutex _nodesLocker;

	ushort _port;
	in_addr _localAddress;
	Socket _socket;
	Socket _broadcastingSocket;

	std::thread _thread;
	std::atomic<bool> _exitFlag;
	
	bool _shouldPing = false;
	sockaddr_in _toPing;

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
	void Start(ushort port = 0, const char* name = nullptr);

	// Stops the running service
	void Stop();

	// Allows to ping given address (with or without a port) to detect running service
	void Ping(const std::string& address);

public:

	// Gets the collection of nodes in the network
	void GetNodes(std::vector<Node*>* output);

	// Tries to find node by the given address, returns null if not found
	Node* GetNode(const sockaddr_in& addr);

private:

	void run();
};
