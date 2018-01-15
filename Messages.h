#pragma once

#include "Config.h"

// Base for all network messages
struct NetworkMsg
{
	byte Type;
	ushort Port;
};

// Maximum size (in bytes) for a single network message
#define MAX_NETWORK_MSG_SIZE 290

// Message types
#define MSG_TYPE_NETOWRK_CHANGE 1
#define MSG_TYPE_REMOVE_FILE 2
#define MSG_TYPE_FIND_FILE 3
#define MSG_TYPE_LIST_FILES 4
#define MSG_TYPE_FILE_INFO 5
#define MSG_TYPE_TRANSFER_REQUEST 6
#define MSG_TYPE_TRANSFER_INIT 7

// Message send after network structure changed (host added/removed)
struct NetworkChangeMsg : NetworkMsg
{
	byte IsNew;
	byte NameLength;
	char Name[MAX_MSG_NAME_LENGTH];
};
static_assert(sizeof(NetworkChangeMsg) <= MAX_NETWORK_MSG_SIZE, "Invalid message size");

// Message send to remove existing file from the network
struct NetworkRemoveFileMsg : NetworkMsg
{
	byte FilenameLength;
	char Filename[MAX_MSG_NAME_LENGTH];
};
static_assert(sizeof(NetworkRemoveFileMsg) <= MAX_NETWORK_MSG_SIZE, "Invalid message size");

// Message send to find file
struct NetworkFindFileMsg : NetworkMsg
{
	byte FilenameLength;
	char Filename[MAX_MSG_NAME_LENGTH];
};
static_assert(sizeof(NetworkFindFileMsg) <= MAX_NETWORK_MSG_SIZE, "Invalid message size");

// Message send to get owned files list
struct NetworkListFilesMsg : NetworkMsg
{
};
static_assert(sizeof(NetworkListFilesMsg) <= MAX_NETWORK_MSG_SIZE, "Invalid message size");

// Message send with owned file information
struct NetworkFileInfoMsg : NetworkMsg
{
	int FileIndex;
	int FilesCount;
	byte FilenameLength;
	char Filename[MAX_MSG_NAME_LENGTH];
	int Size;
	Hash Hash;
};
static_assert(sizeof(NetworkFileInfoMsg) <= MAX_NETWORK_MSG_SIZE, "Invalid message size");

// Message send on file transfer request (node that has file wants to share it)
struct NetworkTransferRequestMsg : NetworkMsg
{
	byte FilenameLength;
	char Filename[MAX_MSG_NAME_LENGTH];
	Hash Hash;
	int Size;
};
static_assert(sizeof(NetworkTransferRequestMsg) <= MAX_NETWORK_MSG_SIZE, "Invalid message size");

// Message send on file data transfer initialization (send by the node requesting data)
struct NetworkTransferInitMsg : NetworkMsg
{
	ushort TcpPort;
	Hash Hash;
};
static_assert(sizeof(NetworkTransferInitMsg) <= MAX_NETWORK_MSG_SIZE, "Invalid message size");
