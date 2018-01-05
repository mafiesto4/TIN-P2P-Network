#include "Service.h"
#include <iostream>

using namespace std;

Service Service::Instance;

void Service::Start(ushort port)
{
	// Skip if already running
	if (IsRunning())
	{
		cout << "Already running" << endl;
		return;
	}

	// Open socket
	_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (_socket == -1)
	{
		cout << "Failed to open a socket" << endl;
		return;
	}

	// Bind local address to the socket
	sockaddr_in localAddr;
	localAddr.sin_family = AF_INET;
	localAddr.sin_addr.s_addr = INADDR_ANY;
	localAddr.sin_port = htons(port);
	int localAddrSize = sizeof(localAddr);
	if (bind(_socket, (sockaddr*)&localAddr, localAddrSize) == -1)
	{
		cout << "Failed to bind address to the socket" << endl;
		return;
	}
	if (getsockname(_socket, (sockaddr*)&localAddr, &localAddrSize) == -1)
	{
		cout << "Failed to get socket name" << endl;
		return;
	}

	// Create a local node
	_local = new Node(localAddr);

	cout << "Running service on localhost, port: " << ntohs(localAddr.sin_port) << endl;
}

void Service::Stop()
{
	// Skip if not runnning
	if (!IsRunning())
	{
		cout << "Not running" << endl;
		return;
	}

	// Close the service socket
#if _WIN32
	closesocket(_socket);
#else
	close(_socket);
#endif
	_socket = -1;

	// Cleanup
	delete _local;
	_local = nullptr;
}
