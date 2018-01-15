#pragma once

#include "Config.h"

// File hosted by the P2P service
class File
{
public:

	// Local file path
	std::string Path;

	// Unique file hash
	Hash Hash;

	// File size (in bytes)
	int Size;
};
