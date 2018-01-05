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
		return getsockname(_descriptor, (struct sockaddr*)result, &size) == -1;
	}

	bool Bind(ushort port)
	{
		sockaddr_in client;
		client.sin_family = AF_INET;
		client.sin_port = htons(port);
		client.sin_addr.s_addr = INADDR_ANY;
		return bind(_descriptor, reinterpret_cast<sockaddr*>(&client), sizeof(client)) == -1;
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

	template<typename T>
	bool Broadcast(ushort port, const T& data)
	{
		return Broadcast(port, (const char*)&data, sizeof(T));
	}

	bool Broadcast(ushort port, const char* data, int length)
	{
		int val = 1;
		if (setsockopt(_descriptor, SOL_SOCKET, SO_BROADCAST, (const char*)&val, sizeof(val)) == -1)
			return true;
		if (setsockopt(_descriptor, SOL_SOCKET, SO_REUSEADDR, (const char*)&val, sizeof(val)) == -1)
			return true;

		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = inet_addr("192.168.0.255");
		//addr.sin_addr.s_addr = INADDR_BROADCAST;
		addr.sin_port = htons(port);

		const int numBytes = sendto(_descriptor, data, length, 0, (struct sockaddr*)&addr, sizeof(addr));
		return numBytes != length;
	}
	
	// Wait for sockets to be ready (any has data to receive)
	static bool Select(Socket** sockets, int count, struct timeval* timeout, Socket** readySockets)
	{
		int sockfd = -1;
		fd_set set;
		FD_ZERO(&set);
		for (int i = 0; i < count; i++)
		{
			sockfd = sockets[i]->_descriptor;
			FD_SET(sockfd, &set);
		}

		int n = select(sockfd + 1, &set, NULL, NULL, timeout);
		if(n == -1)
			return true;

		for (int i = 0; i < count; i++)
		{
			sockfd = sockets[i]->_descriptor;
			Socket* val = nullptr;
			if (FD_ISSET(sockfd, &set))
				val = sockets[i];
			readySockets[i] = val;
		}

		return false;
	}

	// For for single socket to be ready (has data to receive)
	static bool Select(Socket& socket, struct timeval* timeout, bool* isReady)
	{
		fd_set set;
		FD_ZERO(&set);
		int sockfd = socket._descriptor;
		FD_SET(sockfd, &set);

		int n = select(sockfd + 1, &set, NULL, NULL, timeout);
		if (n == -1)
			return true;

		if (isReady)
		{
			sockfd = socket._descriptor;
			*isReady = FD_ISSET(sockfd, &set);
		}

		return false;
	}

	int Receive(char* data, int length, sockaddr_in* sender)
	{
		int senderSize = sizeof(sockaddr_in);
		return recvfrom(_descriptor, data, length, 0, (struct sockaddr*)sender, &senderSize);
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
