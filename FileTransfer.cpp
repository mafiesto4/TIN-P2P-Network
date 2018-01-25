#include "FileTransfer.h"
#include "Socket.h"
#include <iostream>
#include <fstream>
#include "Messages.h"
#include "Service.h"

using namespace std;

bool InputFileTransfer::Perform()
{
#if DETAILED_LOG
	cout << "Start uploading file to " << inet_ntoa(_data.TargetAddress.sin_addr) << endl;
#endif

	// Create TCP socket
	Socket tcpSocket;
	if (tcpSocket.Open(SOCK_STREAM, IPPROTO_TCP) || tcpSocket.Bind(0))
	{
		cout << "Failed to open a socket" << endl;
		return true;
	}
	sockaddr_in tcpAddress;
	tcpSocket.GetAddress(&tcpAddress);

	// Send transfer init message
	{
		NetworkTransferInitMsg msg;
		msg.Type = MSG_TYPE_TRANSFER_INIT;
		msg.Port = Service::Instance.GetPort();
		msg.FileHash = _data.FileHash;
		msg.TcpPort = ntohs(tcpAddress.sin_port);

		Socket udpSocket;
		if (udpSocket.Open(SOCK_DGRAM, IPPROTO_UDP) || udpSocket.Bind(0))
		{
			cout << "Failed to open a socket" << endl;
			return true;
		}

		if (udpSocket.Send(_data.TargetAddress, msg))
		{
			cout << "Failed to initialzie transfer" << endl;
			return true;
		}
	}

	// Wait for the connection
	tcpSocket.Listen();
	Socket msgsock;
	if (tcpSocket.Accept(msgsock))
	{
		return true;
	}

	// Transfer data
	std::vector<char> data(_data.FileSize);
	if(msgsock.ReceiveData(&data[0], (int)data.size()))
	{
		cout << "Failed to get data" << endl;
		return true;
	}

	// Place file in the local database
	if (Service::Instance.AddLocalFile(_data.FileName, _data.FileHash, data))
	{
		cout << "Failed to store file" << endl;
		return true;
	}

	cout << "Recived file \"" << _data.FileName << "\"" << endl;

	return false;
}

bool OutputFileTransfer::Perform()
{
#if DETAILED_LOG
	cout << "Start downloading file from " << inet_ntoa(_data.TargetAddress.sin_addr) << endl;
#endif

	// Create TCP socket
	Socket tcpSocket;
	if (tcpSocket.Open(SOCK_STREAM, IPPROTO_TCP))
	{
		cout << "Failed to open a socket" << endl;
		return true;
	}

	std::this_thread::sleep_for(2ms);

	// Connect
	sockaddr_in targetTcpAddr = _data.TargetAddress;
	targetTcpAddr.sin_port = htons(_data.TcpPort);
	if (tcpSocket.Connect(targetTcpAddr))
	{
		cout << "Failed to connect" << endl;
		return true;
	}

	// Get file data
	auto f = Service::Instance.GetFile(_data.FileHash);
	if (f == nullptr)
	{
		return true;
	}
	std::vector<char> buffer(f->Size);
	{
		// Open file
		std::ifstream file(f->Path.c_str(), std::ios::binary | std::ios::ate);
		if (!file.good())
		{
			cout << "Cannot open file (or invalid path)" << endl;
			return true;
		}

		// Read all data
		const std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);
		if (size <= 0 || size != f->Size)
		{
			cout << "Invalid file size" << endl;
			return true;
		}
		if (!file.read(buffer.data(), size))
		{
			cout << "Failed to load file" << endl;
			return true;
		}
		file.close();
	}

	// Transfer data
	if (tcpSocket.SendData(&buffer[0], (int)buffer.size()))
	{
		cout << "Failed to send data" << endl;
		return true;
	}

	return false;
}
