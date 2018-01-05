#pragma once

#include "Config.h"
#include <string>

// Represents OS socket object
class Socket
{
private:

	int _descriptor;

public:

	Socket(int descriptor = -1)
	{
		_descriptor = descriptor;
	}

	~Socket()
	{
		if (!IsClosed())
			Close();
	}

public:

	bool Open(int type)
	{
		if (!IsClosed())
			return true;
		_descriptor = socket(AF_INET, type, 0);
		return _descriptor == -1;
	}

	bool IsClosed() const
	{
		return _descriptor == -1;
	}

	bool GetAddress(sockaddr_in* result) const
	{
		int size = sizeof(sockaddr_in);
		return getsockname(_descriptor, (sockaddr*)result, &size) == -1;
	}

	bool Bind(ushort port)
	{
		return Bind("0.0.0.0", port);
	}

	bool Bind(const std::string& address, ushort port)
	{
		sockaddr_in client;
		client.sin_family = AF_INET;
		client.sin_port = htons(port);
#if _WIN32
		client.sin_addr.s_addr = inet_addr(address.c_str());
#else
		if (inet_aton(address.c_str(), &client.sin_addr) == 0)
		{
			return true;
		}
#endif
		return bind(_descriptor, reinterpret_cast<sockaddr*>(&client), sizeof(client)) == -1;
	}

	bool Connect(const std::string& address, ushort port)
	{
		sockaddr_in server;
		server.sin_family = AF_INET;
		server.sin_port = htons(port);
#if _WIN32
		server.sin_addr.s_addr = inet_addr(address.c_str());
#else
		if (inet_aton(address.c_str(), &server.sin_addr) == 0)
		{
			return true;
		}
#endif
		return connect(_descriptor, reinterpret_cast<sockaddr*>(&server), sizeof(server)) == -1;
	}

	void Close()
	{
		if (_descriptor != -1)
		{
#if _WIN32
			closesocket(_descriptor);
#else
			close(_descriptor);
#endif
			_descriptor = -1;
		}
	}
};
