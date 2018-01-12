#pragma once

#include "Config.h"

// Incoming data transfer descriptor
struct InputTransferData
{
	sockaddr_in TargetAddress;
	Hash FileHash;
	std::string FileName;
	int FileSize;
};

// Outgoing data transfer descriptor
struct OutputTransferData
{
	sockaddr_in TargetAddress;
	ushort TcpPort;
	Hash FileHash;
};

// Base class for input and output transfers
class FileTransfer
{
	friend class Service;

private:
	
	std::thread _thread;

public:

	// Virtual destrutor
	virtual ~FileTransfer()
	{
	}

public:

	// Checks if the file transfer uses node with the given address
	virtual bool IsNode(const sockaddr_in& addr) const = 0;

	// Checks if the file transfer uses file with the given hash
	virtual bool IsFile(const Hash& hash) const = 0;

	// Starts data transfer, returns true if fails
	virtual bool Perform() = 0;
};

// Incoming data transfer (downloading data from the other node)
class InputFileTransfer : public FileTransfer
{
private:

	InputTransferData _data;

public:

	InputFileTransfer(const InputTransferData& data)
		: _data(data)
	{
	}

public:

	bool IsNode(const sockaddr_in& addr) const override
	{
		return memcmp(&addr, &_data.TargetAddress, sizeof(sockaddr_in)) == 0;
	}
	bool IsFile(const Hash& hash) const override
	{
		return hash == _data.FileHash;
	}
	bool Perform() override;
};

// Outgoing data transfer (uploading data to the other node)
class OutputFileTransfer : public FileTransfer
{
private:

	OutputTransferData _data;

public:

	OutputFileTransfer(const OutputTransferData& data)
		: _data(data)
	{
	}

public:

	bool IsNode(const sockaddr_in& addr) const override
	{
		return memcmp(&addr, &_data.TargetAddress, sizeof(sockaddr_in)) == 0;
	}
	bool IsFile(const Hash& hash) const override
	{
		return hash == _data.FileHash;
	}
	bool Perform() override;
};
