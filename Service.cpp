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
	if (_socket.Open(SOCK_DGRAM))
	{
		cout << "Failed to open a socket" << endl;
		return;
	}

	// Bind local address to the socket
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
	_local = new Node(addr);

	cout << "Running service on localhost:" << ntohs(addr.sin_port) << endl;
}

void Service::Stop()
{
	// Skip if not runnning
	if (!IsRunning())
	{
		cout << "Not running" << endl;
		return;
	}

	// Cleanup
	_socket.Close();
	delete _local;
	_local = nullptr;
}
