#include "FileTransfer.h"
#include "Socket.h"
#include <iostream>
#include "Messages.h"
#include "Service.h"

using namespace std;

bool InputFileTransfer::Perform()
{
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
		msg.Hash = _data.FileHash;
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

	// Transfer data
	Socket msgsock;
	if (tcpSocket.Accept(msgsock))
	{
		return true;
	}
	char data[32];
	if(msgsock.ReceiveData(data, 32))
	{
		cout << "Failed to get data" << endl;
		return true;
	}

	return false;
}

bool OutputFileTransfer::Perform()
{
	// Create TCP socket
	Socket tcpSocket;
	if (tcpSocket.Open(SOCK_STREAM, IPPROTO_TCP))
	{
		cout << "Failed to open a socket" << endl;
		return true;
	}

	// Connect
	sockaddr_in targetTcpAddr = _data.TargetAddress;
	targetTcpAddr.sin_port = htons(_data.TcpPort);
	if(tcpSocket.Connect(targetTcpAddr))
	{
		cout << "Failed to connect" << endl;
		return true;
	}

	// TODO: get file data
	char data[32];
	for (int i = 0; i < 32; i++)
		data[i] = i;

	// Transfer data
	if (tcpSocket.SendData(data, 32))
	{
		cout << "Failed to send data" << endl;
		return true;
	}

	return false;
}
