#pragma once

#include "Node.h"
#include "Socket.h"
#include "FileTransfer.h"
#include "File.h"
#include <queue>

// The main peer-to-peer server controller. Exposes various functionalities and manages the network connections and structure.
class Service
{
	friend class InputFileTransfer;

public:

	// The global instance of the service (singleton)
	static Service Instance;

private:

	Node* _local = nullptr;
	std::vector<Node*> _nodes;
	std::recursive_mutex _nodesLocker;

	ushort _port = 0;
	in_addr _localAddress;
	Socket _socket;
	Socket _broadcastingSocket;

	std::thread _thread;
	std::atomic<bool> _exitFlag;
	
	bool _shouldPing = false;
	sockaddr_in _toPing;

	bool _shouldListFiles = false;
	std::string _listHostname;

	std::queue<InputTransferData> _inputData;
	std::queue<OutputTransferData> _outputData;
	std::vector<FileTransfer*> _activeTransfers;
	std::vector<FileTransfer*> _endedTransfers;
	std::recursive_mutex _transferLocker;

	std::string _localFilesDatabasePath;
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

	// Gets the service port number
	ushort GetPort() const
	{
		return _port;
	}

public:

	// Starts the service at the given port (uses the local address)
	void Start(ushort port = 0, const char* name = nullptr);

	// Stops the running service
	void Stop();

	// Allows to ping given address (with or without a port) to detect running service
	void Ping(const std::string& address);

	// Adds new file to the network
	void AddFile(const std::string& filename);

	// Sends a message to the connected nodes or choosen host (if valid) to recive files list
	void ListFiles(const std::string& hostName);

public:

	// Gets the collection of nodes in the network
	void GetNodes(std::vector<Node*>* output);

	// Tries to find node by the given address, returns null if not found
	Node* GetNode(const sockaddr_in& addr);

	// Tries to find node by the given name, returns null if not found
	Node* GetNode(const std::string& hostname);

	// Tries to find local file by the given file hash, returns null if not found
	File* GetFile(const Hash& hash);

	// Checks if file with given hash is during transfer (input or output)
	bool IsFileTransfer(const Hash& hash);

private:

	template<typename T>
	bool Broadcast(const T& data)
	{
		return Broadcast((const char*)&data, sizeof(T));
	}

	bool Broadcast(const char* data, int length);

	void run();
	void runTransfer(FileTransfer* transfer);
	void SendFile(const OutputTransferData& data);
	void GetFile(const InputTransferData& data);
	void OnTransferStart(FileTransfer* transfer);
	void OnTransferEnd(FileTransfer* transfer);
	void HandleEndedTransfers();
	void HandleFiles();
	bool AddLocalFile(const std::string& filename, const Hash& hash, std::vector<char>& data);
	bool StoreLocalFileData(File* file, std::vector<char>& data);
	void UpdateLocalFiles();
};
