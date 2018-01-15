#include "Service.h"
#include <iostream>
#include "Messages.h"
#include <fstream>
#include "md5.h"
#include <filesystem>
#include <stdio.h>
#include <time.h>

using namespace std;

Service Service::Instance;

// Computes hashed value for the target node index that should store the file of the given hash (kind of hash from hash)
int File2NodeHash(const Hash& hash, int nodesCount)
{
	int v = hash.Data[0];
	for (int i = 1; i < 16; i++)
		v = (v * 379) ^ hash.Data[0];
	return v % nodesCount;
}

bool ParseAddress(const std::string& address, sockaddr_in& addr)
{
	// Check single port case
	{
		char *endptr;
		const long val = strtol(address.c_str(), &endptr, 10);
		if (*endptr == '\0' && val > 0 && val <= MAX_PORT_NUM)
		{
			addr.sin_port = htons((u_short)val);
			return false;
		}
	}

	string pAddr;
	string pPort;

	// Try to get address and port values
	const string::size_type split = address.find(':');
	if (split == string::npos)
	{
		pAddr = address;
		pPort = "";
	}
	else
	{
		pAddr = address.substr(0, split);
		pPort = address.substr(split + 1);
	}

	// Try to parse address
	if (!pAddr.empty() && inet_pton(AF_INET, pAddr.c_str(), &addr.sin_addr) != 1)
	{
		return true;
	}

	if (!pPort.empty())
	{
		// Try parse port
		char *endptr;
		const long val = strtol(pPort.c_str(), &endptr, 10);
		if (*endptr != '\0' || val <= 0 || val > MAX_PORT_NUM)
			return true;

		addr.sin_port = htons((u_short)val);
	}

	return false;
}

bool GetLocalAddress(in_addr* result)
{
	char szBuffer[1024];

	if (gethostname(szBuffer, sizeof(szBuffer)) == SOCKET_ERROR)
		return true;

	struct hostent* host = gethostbyname(szBuffer);
	if (host == NULL)
		return true;

	*result = *((struct in_addr*)(host->h_addr));

	return false;
}

void Service::Start(ushort port, const char* name)
{
	// Skip if already running
	if (IsRunning())
	{
		cout << "Already running" << endl;
		return;
	}

	// Peek default name if none specified
	std::string nodeName;
	if (name == nullptr)
	{
#if _WIN32
		char hostname[256];
		DWORD bufCharCount = 256;
		GetComputerNameA(hostname, &bufCharCount);
		nodeName = hostname;
		nodeName += " (" + std::to_string(GetCurrentProcessId()) + ")";
#else
		char hostname[HOST_NAME_MAX];
		gethostname(hostname, HOST_NAME_MAX);
		nodeName = hostname;
		nodeName += " (" + std::to_string(getpid()) + ")";
#endif
		name = nodeName.c_str();
	}

	// Get local address
	if (GetLocalAddress(&_localAddress))
	{
		cout << "Failed to get local address" << endl;
		return;
	}

	// Setup local files database
	// TODO: better local database folder name or sth
	_localFilesDatabasePath = nodeName;
	std::experimental::filesystem::create_directory(_localFilesDatabasePath);

	// Open sockets
	if (_socket.Open(SOCK_DGRAM, IPPROTO_UDP) || _broadcastingSocket.Open(SOCK_DGRAM, IPPROTO_UDP))
	{
		cout << "Failed to open a socket" << endl;
		return;
	}

	// Bind local address to the sending socket
	if (_socket.Bind(_localAddress, port))
	{
		cout << "Failed to bind address to the socket" << endl;
		return;
	}
	sockaddr_in addr;
	if (_socket.GetAddress(&addr))
	{
		cout << "Failed to get socket name" << endl;
		return;
	}

	// Create a local node
	_local = new Node(addr, name);
	_nodes.push_back(_local);
	_port = ntohs(addr.sin_port);

	// Cleanup
	_inputData = vector<InputTransferData>();
	_outputData = vector<OutputTransferData>();

	cout << "Running service on localhost:" << _port << endl;
	cout << "Name: " << _local->GetName() << endl;

	// Start the service thread
	_exitFlag.store(false);
	std::thread t(&Service::run, this);
	_thread = move(t);
}

void Service::Stop()
{
	// Skip if not runnning
	if (!IsRunning())
	{
		cout << "Not running" << endl;
		return;
	}

	// Stop the worker thread
	_exitFlag.store(true);
	_thread.join();

	// Close network connections
	_socket.Close();
	_broadcastingSocket.Close();

	// Remove local files
	for (auto& file : _files)
		delete file;
	_files.clear();
	std::experimental::filesystem::remove_all(_localFilesDatabasePath);
	_localFilesDatabasePath.clear();

	// Cleanup
	for (auto& _node : _nodes)
		delete _node;
	_nodes.clear();
	_local = nullptr;
	_port = 0;

	cout << "Stopped" << endl;
}

void Service::Ping(const std::string& address)
{
	// Skip if not running
	if (!IsRunning())
	{
		cout << "Not running" << endl;
		return;
	}
	if (_shouldPing)
		return;

	// Parse address
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr = _localAddress;
	addr.sin_port = htons(0);
	if(ParseAddress(address, addr))
	{
		cout << "Invalid address" << endl;
		return;
	}

	_toPing = addr;
	_shouldPing = true;
}

void CalculateHash(std::vector<char>& data, Hash& hash)
{
	// Use MD5
	MD5 md5 = MD5(data);
	md5.get(hash.Data);
}

bool CompareNodes(Node* a, Node* b)
{
	auto x = a->GetAddress();
	auto y = b->GetAddress();
	if (memcpy(&x.sin_addr, &y.sin_addr, sizeof(in_addr)) < 0)
		return true;
	return x.sin_port < y.sin_port;
}

void Service::AddFile(const std::string& filename)
{
	// Open file
	std::ifstream file(filename.c_str(), std::ios::binary | std::ios::ate);
	if(!file.good())
	{
		cout << "Cannot open file (or invalid path)" << endl;
		return;
	}

	// Read all data
	const std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);
	if(size <= 0)
	{
		cout << "Cannot add empty file" << endl;
		return;
	}
	std::vector<char> buffer(size);
	if (!file.read(buffer.data(), size))
	{
		cout << "Failed to load file" << endl;
		return;
	}
	file.close();

	// Create new file (local)
	auto f = new File();
	f->Filename = filename;
	f->Size = buffer.size();
	CalculateHash(buffer, f->Hash);

	// Place file in the local database
	if(StoreLocalFileData(f, buffer))
	{
		delete f;
		cout << "Failed to store file" << endl;
		return;
	}

	// Update files
	{
		scope_lock lock(_filesLocker);
		_files.push_back(f);
		cout << "File \"" << filename << "\" added!" << endl;

		// Redistribute resources
		UpdateLocalFiles();
	}
}

void Service::RemoveFile(const std::string& filename)
{
	// Skip if not running
	if (!IsRunning())
	{
		cout << "Not running" << endl;
		return;
	}
	if (_shouldRemoveFile)
		return;

	_removeFilename = filename;
	_shouldRemoveFile = true;
}

void Service::DownloadFile(const std::string& filename, const std::string& localPath)
{
	// Skip if not running
	if (!IsRunning())
	{
		cout << "Not running" << endl;
		return;
	}
	if (_shouldGetFile)
		return;

	_getFilename = filename;
	_getFileLocalPath = localPath;
	_shouldGetFile = true;
	_getFileWaitForInfo = false;
}

void Service::ListFiles(const std::string& hostName)
{
	// Skip if not running
	if (!IsRunning())
	{
		cout << "Not running" << endl;
		return;
	}
	if (_shouldListFiles)
		return;

	_listHostname = hostName;
	_getFileWaitForInfo = false;
	_shouldListFiles = true;
}

void Service::GetNodes(std::vector<Node*>* output)
{
	assert(output);

	scope_lock lock(_nodesLocker);
	*output = _nodes;
}

Node* Service::GetNode(const sockaddr_in& addr)
{
	scope_lock lock(_nodesLocker);

	for(auto& node : _nodes)
	{
		if (node->GetAddress() == addr)
			return node;
	}

	return nullptr;
}

Node* Service::GetNode(const std::string& hostname)
{
	scope_lock lock(_nodesLocker);

	for (auto& node : _nodes)
	{
		if (node->GetName() == hostname)
			return node;
	}

	return nullptr;
}

File* Service::GetFile(const Hash& hash)
{
	scope_lock lock(_filesLocker);

	for (auto& file : _files)
	{
		if (file->Hash == hash)
			return file;
	}

	return nullptr;
}

File* Service::GetFile(const std::string& filename)
{
	scope_lock lock(_filesLocker);

	for (auto& file : _files)
	{
		if (file->Filename == filename)
			return file;
	}

	return nullptr;
}

bool Service::IsFileTransfer(const Hash& hash)
{
	scope_lock lock(_transferLocker);

	// Check active transfers
	for (auto& transfer : _activeTransfers)
	{
		if (transfer->IsFile(hash))
			return true;
	}

	// Check queued transfers
	for (auto& data : _inputData)
	{
		if (data.FileHash == hash)
			return true;
	}
	for (auto& data : _outputData)
	{
		if (data.FileHash == hash)
			return true;
	}

	return false;
}

bool Service::Broadcast(const char* data, int length)
{
	if (_broadcastingSocket.Broadcast(_port, data, length))
	{
		return true;
	}

	// Also send ending message to the connected nodes that use different service port
	{
		scope_lock lock(_nodesLocker);

		const u_short port = htons(_port);
		for (auto& node : _nodes)
		{
			if (node->GetAddress().sin_port != port)
			{
				if (_socket.Send(node->GetAddress(), data, length))
					return true;
			}
		}
	}

	return false;
}

void Service::run()
{
	assert(!_socket.IsClosed());

	NetworkChangeMsg thisChangeMsg;
	thisChangeMsg.Type = MSG_TYPE_NETOWRK_CHANGE;
	thisChangeMsg.IsNew = 1;
	thisChangeMsg.Port = _port;
	thisChangeMsg.NameLength = (byte)strlen(_local->GetName());
	memcpy(thisChangeMsg.Name, _local->GetName(), thisChangeMsg.NameLength + 1);

	// Broadcast about new node in the network
	if (_broadcastingSocket.Broadcast(_port, thisChangeMsg))
	{
		cout << "Failed to send an initial broadcast message" << endl;
	}

	sockaddr_in sender;
	bool isReady;
	struct timeval time_500ms = { 0, 500 * 1000 };
	char buffer[MAX_NETWORK_MSG_SIZE];

	while (!_exitFlag.load())
	{
		std::this_thread::sleep_for(1ms);

		HandleEndedTransfers();
		HandleNewTransfers();
		HandleDownloadFile();
		HandleFilesLocality();
		HandleFiles();

		// Ping if need
		if (_shouldPing)
		{
			if (_socket.Send(_toPing, thisChangeMsg))
			{
				cout << "Failed to send ping message" << endl;
			}
			_shouldPing = false;
		}

		// List files if need
		if (_shouldListFiles)
		{
			// TODO: this should be done in a better way (create eparate socket/thread and use it for handling this)
			NetworkListFilesMsg msg;
			msg.Type = MSG_TYPE_LIST_FILES;
			msg.Port = _port;
			bool result;
			const auto node = GetNode(_listHostname);
			if (node)
				result = _socket.Send(node->GetAddress(), msg);
			else
				result = Broadcast(msg);
			if (result)
			{
				cout << "Failed to send file list message" << endl;
			}

			// Print local files
			{
				cout << "Files:" << endl;
				auto thisNode = GetLocalNode();
				scope_lock lock(_filesLocker);
				for (auto& file : _files)
					cout << thisNode->GetName() << ": " << file->Filename << endl;
			}

			_shouldListFiles = false;
		}

		// Wait for a message (non blocking)
		if (Socket::Select(_socket, &time_500ms, &isReady))
		{
			cout << "Failed to select ready socket" << endl;
		}
		if (isReady)
		{
			const int size = _socket.Receive(buffer, MAX_NETWORK_MSG_SIZE, &sender);
			if (size == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
			{
				cout << "Failed to read data" << endl;
			}
			else if (size >= sizeof(NetworkMsg))
			{
				// Override sender port address
				NetworkMsg* msgBase = (NetworkMsg*)buffer;
				sender.sin_port = htons(msgBase->Port);

				// Skip messages from itself
				if (memcmp(&_localAddress, &sender.sin_addr, sizeof(sender.sin_addr)) == 0 && msgBase->Port == _port)
					continue;

				// Handle message
				switch (msgBase->Type)
				{
				case MSG_TYPE_NETOWRK_CHANGE:
				{
					NetworkChangeMsg* msg = (NetworkChangeMsg*)msgBase;

					scope_lock lock(_nodesLocker);
					if (msg->IsNew)
					{
						// Create new node (only if missing)
						auto node = GetNode(sender);
						if (node == nullptr)
						{
							node = new Node(sender, msg->Name);
							_nodes.push_back(node);

							// Keep nodes sorted
							std::sort(_nodes.begin(), _nodes.end(), CompareNodes);

							// Send back message
							if (_socket.Send(sender, thisChangeMsg))
							{
								cout << "Failed to send message to the new node" << endl;
							}

							cout << "Node connected: " << node << endl;

							// Redistribute resources
							UpdateLocalFiles();
						}
					}
					else
					{
						// Remove node (only if existing)
						auto node = GetNode(sender);
						if (node)
						{
							cout << "Node disconnected: " << node << endl;

							_nodes.erase(std::find(_nodes.begin(), _nodes.end(), node));
							delete node;

							// Redistribute resources
							UpdateLocalFiles();
						}
					}

					break;
				}
				case MSG_TYPE_REMOVE_FILE:
				{
					NetworkRemoveFileMsg* msg = (NetworkRemoveFileMsg*)msgBase;
					const std::string filename = msg->Filename;

					scope_lock lock(_filesLocker);

					// Check if has that file
					for (auto& file : _files)
					{
						if (file->Filename == filename)
						{
							cout << "Removing file \"" << filename << "\"" << endl;

							// Mark to remove (will be deleted in HandleFiles if not used anymore)
							file->MarkedToRemove = true;
							file->TTL = 0.5f;
							break;
						}
					}

					break;
				}
				case MSG_TYPE_FIND_FILE:
				{
					NetworkFindFileMsg* msg = (NetworkFindFileMsg*)msgBase;
					const std::string filename = msg->Filename;

					scope_lock lock(_filesLocker);

					// Check if has that file
					int index = 0;
					for (auto& file : _files)
					{
						index++;
						if (file->Filename == filename)
						{
							NetworkFileInfoMsg msg;
							msg.Type = MSG_TYPE_FILE_INFO;
							msg.Port = _port;
							msg.FileIndex = index;
							msg.FilesCount = _files.size();
							msg.FilenameLength = file->Filename.size();
							memcpy(msg.Filename, file->Filename.c_str(), msg.FilenameLength + 1);
							msg.Filename[msg.FilenameLength] = 0;
							msg.Size = file->Size;
							msg.Hash = file->Hash;
							if (_socket.Send(sender, msg))
							{
								cout << "Failed to send file info message" << endl;
							}

							break;
						}
					}

					break;
				}
				case MSG_TYPE_LIST_FILES:
				{
					scope_lock lock(_filesLocker);

					// Send back all owned files
					int index = 0;
					for (auto& file : _files)
					{
						NetworkFileInfoMsg msg;
						msg.Type = MSG_TYPE_FILE_INFO;
						msg.Port = _port;
						msg.FileIndex = index++;
						msg.FilesCount = _files.size();
						msg.FilenameLength = file->Filename.size();
						memcpy(msg.Filename, file->Filename.c_str(), msg.FilenameLength + 1);
						msg.Filename[msg.FilenameLength] = 0;
						msg.Size = file->Size;
						msg.Hash = file->Hash;
						if (_socket.Send(sender, msg))
						{
							cout << "Failed to send file info message" << endl;
						}
					}

					break;
				}
				case MSG_TYPE_FILE_INFO:
				{
					NetworkFileInfoMsg* msg = (NetworkFileInfoMsg*)msgBase;

					// Validate sender
					const auto node = GetNode(sender);
					if (node == nullptr)
					{
						cout << "File transfer request from unknown node" << endl;
						break;
					}

					// Check if it's file to get
					if(_getFileWaitForInfo && msg->Filename == _getFilename)
					{
						// Get this file
						InputTransferData data;
						data.TargetAddress = sender;
						data.FileSize = msg->Size;
						data.FileName = msg->Filename;
						data.FileHash = msg->Hash;
						GetFile(data);
					}
					else
					{
						// Print file info
						cout << node->GetName() << ": " << msg->Filename << endl;
					}

					break;
				}
				case MSG_TYPE_TRANSFER_REQUEST:
				{
					NetworkTransferRequestMsg* msg = (NetworkTransferRequestMsg*)msgBase;

					// Validate sender
					const auto node = GetNode(sender);
					if (node == nullptr)
					{
						cout << "File transfer request from unknown node" << endl;
						break;
					}

					// Validate if file has been already added to this node (or is empty)
					if (GetFile(msg->Hash) == nullptr || msg->Size <= 0)
					{
						// Push reciving data request
						InputTransferData data;
						data.TargetAddress = sender;
						data.FileSize = msg->Size;
						data.FileName = msg->Filename;
						data.FileHash = msg->Hash;
						GetFile(data);
					}

					break;
				}
				case MSG_TYPE_TRANSFER_INIT:
				{
					NetworkTransferInitMsg* msg = (NetworkTransferInitMsg*)msgBase;

					// Validate sender
					const auto node = GetNode(sender);
					if (node == nullptr)
					{
						cout << "File transfer request from unknown node" << endl;
						break;
					}

					// Push sending data request
					OutputTransferData data;
					data.TargetAddress = sender;
					data.TcpPort = msg->TcpPort;
					data.FileHash = msg->Hash;
					SendFile(data);

					break;
				}
				default:
					cout << "Unknown network message type (" << msgBase->Type << ")" << endl;
				}
			}
		}
	}

	// Broadcast about node leaving the network
	thisChangeMsg.IsNew = 0;
	if (Broadcast(thisChangeMsg))
	{
		cout << "Failed to send an ending broadcast message" << endl;
	}
}

void Service::runTransfer(FileTransfer* transfer)
{
	assert(transfer);

	const bool result = transfer->Perform();
	if (result)
	{
		cout << "Transfer failed" << endl;
	}

	OnTransferEnd(transfer);
}

void Service::SendFile(const OutputTransferData& data)
{
	scope_lock lock(_transferLocker);

	// Check if can start new transfer
	if (_activeTransfers.size() < MAX_ACTIVE_TRANSFERS_COUNT)
	{
		// Kick off new file data transfer
		OnTransferStart(new OutputFileTransfer(data));
		return;
	}

	// Store data to handle it later
	_outputData.push_back(data);
}

void Service::GetFile(const InputTransferData& data)
{
	scope_lock lock(_transferLocker);

	// Check if can start new transfer
	if (_activeTransfers.size() < MAX_ACTIVE_TRANSFERS_COUNT)
	{
		// Kick off new file data transfer
		OnTransferStart(new InputFileTransfer(data));
		return;
	}

	// Store data to handle it later
	_inputData.push_back(data);
}

void Service::OnTransferStart(FileTransfer* transfer)
{
	_activeTransfers.push_back(transfer);
	std::thread t(&Service::runTransfer, this, transfer);
	transfer->_thread = move(t);
}

void Service::OnTransferEnd(FileTransfer* transfer)
{
	assert(transfer);

	scope_lock lock(_transferLocker);
	_endedTransfers.push_back(transfer);
}

void Service::HandleEndedTransfers()
{
	do
	{
		FileTransfer* transfer = nullptr;
		{
			scope_lock lock(_transferLocker);
			if (!_endedTransfers.empty())
			{
				auto i = _endedTransfers.begin();
				transfer = *i;
				_endedTransfers.erase(i);
				i = find(_activeTransfers.begin(), _activeTransfers.end(), transfer);
				assert(i != _activeTransfers.end());
				_activeTransfers.erase(i);
			}
		}
		if (transfer == nullptr)
			break;

		// Wait for thread end
		transfer->_thread.join();

		// Cleanup
		delete transfer;

	} while (true);
}

void Service::HandleNewTransfers()
{
	scope_lock lock(_transferLocker);

	// Try to start new file transfers from the waiting queue
	while (!_inputData.empty() && _activeTransfers.size() < MAX_ACTIVE_TRANSFERS_COUNT)
	{
		GetFile(_inputData.at(0));
		_inputData.erase(_inputData.begin());
	}
	while (!_outputData.empty() && _activeTransfers.size() < MAX_ACTIVE_TRANSFERS_COUNT)
	{
		SendFile(_outputData.at(0));
		_outputData.erase(_outputData.begin());
	}
}

void Service::HandleFiles()
{
	scope_lock lock(_filesLocker);

	// Update TTL
	static time_t LastUpdate = 0;
	time_t seconds = time(NULL);
	if (LastUpdate == 0)
		LastUpdate = seconds;
	time_t diff = seconds - LastUpdate;
	LastUpdate = seconds;
	if (diff > 0)
	{
		for (auto& file : _files)
		{
			if (file->TTL > 0)
				file->TTL -= diff;
		}
	}

	// Check if remove file
	if (_shouldRemoveFile)
	{
		// Search local files
		for (auto& file : _files)
		{
			if (file->Filename == _removeFilename)
			{
				cout << "Removing file \"" << _removeFilename << "\"" << endl;

				// Mark to remove
				file->MarkedToRemove = true;
				file->TTL = 0.5f;
				break;
			}
		}

		// Send message to the other nodes
		NetworkRemoveFileMsg msg;
		msg.Type = MSG_TYPE_REMOVE_FILE;
		msg.Port = _port;
		msg.FilenameLength = _removeFilename.size();
		memcpy(msg.Filename, _removeFilename.c_str(), msg.FilenameLength + 1);
		msg.Filename[msg.FilenameLength] = 0;
		if (Broadcast(msg))
		{
			cout << "Failed to send remove file message" << endl;
		}

		_shouldRemoveFile = false;
	}

	// Remove not used remote files and not used files marked to delete
	std::vector<File*> toRemove;
	for (auto& file : _files)
	{
		if (file->TTL <= 0.0f && (!file->IsLocal || file->MarkedToRemove) && !IsFileTransfer(file->Hash))
		{
			// File is not used in any transfer and it's signed to the other node so remove the local data
			toRemove.push_back(file);
		}
	}
	for (auto& file : toRemove)
	{
		_files.erase(std::find(_files.begin(), _files.end(), file));
		if (!file->Path.empty())
		{
			std::experimental::filesystem::remove(file->Path);
		}
		delete file;
	}
}

void Service::HandleFilesLocality()
{
	// Skip if flag is empty
	if (!_updateLocalFiles)
		return;
	_updateLocalFiles = false;

	// This method checks all local files to see if need to transfer any of them to the another nodes
	// Warning: it locks both files and nodes lists, should not introducte deadlocks
	// Note: this assumes that nodes are sorted by the IP address and port - deterministic on all hosts

	scope_lock lock1(_filesLocker);
	scope_lock lock2(_nodesLocker);
	const int nodesCnt = _nodes.size();

	for (auto& file : _files)
	{
		const int node = File2NodeHash(file->Hash, nodesCnt);
		assert(node >= 0 && node < nodesCnt);
		const bool isLocal = _nodes[node] == GetLocalNode();
		file->IsLocal = isLocal;
		if (!isLocal)
		{
			// Give some time until removing this file
			file->TTL = 2.0f;

			// Notify node about file it should have
			NetworkTransferRequestMsg msg;
			msg.Type = MSG_TYPE_TRANSFER_REQUEST;
			msg.Port = _port;
			msg.FilenameLength = file->Filename.size();
			memcpy(msg.Filename, file->Filename.c_str(), msg.FilenameLength + 1);
			msg.Filename[msg.FilenameLength] = 0;
			msg.Hash = file->Hash;
			msg.Size = file->Size;
			if (_socket.Send(_nodes[node]->GetAddress(), msg))
			{
				cout << "Failed to send transfer notify message" << endl;
			}
		}
	}
}

void Service::HandleDownloadFile()
{
	// Get file if need
	if (!_shouldGetFile)
		return;
	_shouldGetFile = false;

	// Check itself
	{
		scope_lock lock(_filesLocker);
		const auto file = GetFile(_getFilename);
		if (file && !file->Path.empty())
		{
			cout << "Getting local file" << endl;
			std::experimental::filesystem::copy_file(file->Path, _getFileLocalPath, experimental::filesystem::copy_options::update_existing);
			return;
		}
	}

	// Find this file (just ask other nodes for this file)
	NetworkFindFileMsg msg;
	msg.Type = MSG_TYPE_FIND_FILE;
	msg.Port = _port;
	msg.FilenameLength = _getFilename.size();
	memcpy(msg.Filename, _getFilename.c_str(), msg.FilenameLength + 1);
	msg.Filename[msg.FilenameLength] = 0;
	if(Broadcast(msg))
	{
		cout << "Failed to send find file message" << endl;
		return;
	}

	_getFileWaitForInfo = true;
}

bool Service::AddLocalFile(const std::string& filename, const Hash& hash, std::vector<char>& data)
{
	// Check if it's file to get
	if(_getFileWaitForInfo && filename == _getFilename)
	{
		cout << "Downloaded file" << endl;
		ofstream outfile(_getFileLocalPath, ios::out | ios::binary);
		outfile.write(&data[0], data.size());
		_getFileWaitForInfo = false;
		_getFilename.clear();
		_getFileLocalPath.clear();
		_shouldGetFile = false;
		return false;
	}

	// Create new file (local)
	auto f = new File();
	f->Filename = filename;
	f->Size = data.size();
	f->Hash = hash;

	// Place file in the local database
	if (StoreLocalFileData(f, data))
	{
		delete f;
		return true;
	}

	// Update files
	{
		scope_lock lock(_filesLocker);
		_files.push_back(f);
	}

	return false;
}

bool Service::StoreLocalFileData(File* file, std::vector<char>& data)
{
	// Evaluate local file path (in database)
	file->Path = _localFilesDatabasePath + '/' + file->Filename;

	// Serialize data
	ofstream outfile(file->Path.c_str(), ios::out | ios::binary);
	outfile.write(&data[0], data.size());

	return false;
}

void Service::UpdateLocalFiles()
{
	// Set flag
	_updateLocalFiles = true;
}
