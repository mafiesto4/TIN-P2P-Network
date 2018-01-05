#include "Service.h"
#include <iostream>
#include "Messages.h"

using namespace std;

Service Service::Instance;

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

	// Open sockets
	if (_socket.Open(SOCK_DGRAM) || _broadcastingSocket.Open(SOCK_DGRAM))
	{
		cout << "Failed to open a socket" << endl;
		return;
	}

	// Bind local address to the sending socket
	if (_socket.Bind(port))
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

	// Cleanup
	for (auto& _node : _nodes)
		delete _node;
	_nodes.clear();
	_local = nullptr;
	_port = 0;

	cout << "Stopped" << endl;
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
		sockaddr_in nodeAddr = node->GetAddress();
		if (memcmp(&addr, &nodeAddr, sizeof(sockaddr_in)) == 0)
			return node;
	}

	return nullptr;
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

void Service::run()
{
	assert(!_socket.IsClosed());

	// Broadcast about new node in the network
	{
		NetworkChangeMsg msg;
		msg.Type = MSG_TYPE_NETOWRK_CHANGE;
		msg.IsNew = 1;
		msg.NameLength = (byte)strlen(_local->GetName());
		memcpy(msg.Name, _local->GetName(), msg.NameLength + 1);

		if(_broadcastingSocket.Broadcast(_port, msg))
		{
			cout << "Failed to send an initial broadcast message" << endl;
		}
	}

	// Get local address
	in_addr localAddr;
	if(GetLocalAddress(&localAddr))
	{
		cout << "Failed to get local address" << endl;
	}
	
	sockaddr_in sender;
	bool isReady;
	struct timeval time_500ms = { 0, 500 * 1000 };
	char buffer[MAX_NETWORK_MSG_SIZE];

	while (!_exitFlag.load())
	{
		std::this_thread::sleep_for(1ms);

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
				// Skip messages from itself
				if (memcmp(&localAddr, &sender.sin_addr, sizeof(sender.sin_addr)) == 0)
					continue;

				// Handle message
				switch (((NetworkMsg*)buffer)->Type)
				{
				case MSG_TYPE_NETOWRK_CHANGE:
				{
					NetworkChangeMsg* msg = (NetworkChangeMsg*)buffer;

					cout << "Network change: " << msg->Name << endl;

					scope_lock lock(_nodesLocker);
					if (msg->IsNew)
					{
						// Create new node (only if missing)
						auto node = GetNode(sender);
						if (node == nullptr)
						{
							_nodes.push_back(new Node(sender, msg->Name));
						}
					}
					else
					{
						// Remove node (only if existing)
						auto node = GetNode(sender);
						if (node)
						{
							_nodes.erase(std::find(_nodes.begin(), _nodes.end(), node));
							delete node;
						}
					}

					break;
				}
				default:
					cout << "Unknown network message type" << endl;
				}
			}
		}
	}

	// Broadcast about node leaving the network
	{
		NetworkChangeMsg msg;
		msg.Type = MSG_TYPE_NETOWRK_CHANGE;
		msg.IsNew = 0;
		msg.NameLength = (byte)strlen(_local->GetName());
		memcpy(msg.Name, _local->GetName(), msg.NameLength + 1);

		if (_broadcastingSocket.Broadcast(_port, msg))
		{
			cout << "Failed to send an ending broadcast message" << endl;
		}
	}
}
