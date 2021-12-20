#pragma once

#include <raylib.h>

namespace Dithering
{
	// Initializes the required lua state
	void LuaInit(Image& image);
	// Executes the script and cleans up after lua
	void LuaExecute(const char* scriptPath);
}