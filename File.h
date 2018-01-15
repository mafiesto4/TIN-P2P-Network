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
};
