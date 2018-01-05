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

	// Cleanup
	_socket.Close();
	delete _local;
	_local = nullptr;
}

void Service::run()
{
	while (!_exitFlag.load())
	{
		std::this_thread::sleep_for(1ms);
	}
}
