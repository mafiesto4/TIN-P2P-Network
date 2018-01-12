#pragma once

#include "Node.h"
#include "Socket.h"
#include "FileTransfer.h"
#include "File.h"
#include <queue>

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

	std::queue<InputTransferData> _inputData;
	std::queue<OutputTransferData> _outputData;
	std::vector<FileTransfer*> _activeTransfers;
	std::vector<FileTransfer*> _endedTransfers;
	std::recursive_mutex _transferLocker;

	std::vector<File*> _files;
	std::recursive_mutex _filesLocker;

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

	// Tries to find local file by the given file hash, returns null if not found
	File* GetFile(const Hash& hash);

public:

	// Testing function, remove it later
	void SendTestTransferToItself();

private:

	void run();
	void runTransfer(FileTransfer* transfer);
	void SendFile(const OutputTransferData& data);
	void GetFile(const InputTransferData& data);
	void OnTransferStart(FileTransfer* transfer);
	void OnTransferEnd(FileTransfer* transfer);
	void HandleEndedTransfers();
};
