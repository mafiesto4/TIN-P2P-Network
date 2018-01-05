#include "Service.h"
#include <iostream>

using namespace std;

Service Service::Instance;

void Service::Start(const char* name, ushort port)
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
	_local = new Node(addr, name);

	cout << "Running service on localhost:" << ntohs(addr.sin_port) << endl;
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

	// Cleanup
	_socket.Close();
	delete _local;
	_local = nullptr;
}

void Service::run()
{
	assert(!_socket.IsClosed());

	while (!_exitFlag.load())
	{
		std::this_thread::sleep_for(1ms);
	}
}
