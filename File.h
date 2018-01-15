#pragma once

#include "Config.h"

// File hosted by the P2P service
class File
{
public:

	// Local file path
	std::string Path;

	// File name used inside the network (for add/remove/get requests)
	std::string Filename;

	// Unique file hash
	Hash Hash;

	// File size (in bytes)
	int Size;

	// True if file should be stored in the local node, otherwise false if send it to the other node (not used remove files are removed)
	bool IsLocal = true;

	// True if remove file if not used
	bool MarkedToRemove = false;
};
