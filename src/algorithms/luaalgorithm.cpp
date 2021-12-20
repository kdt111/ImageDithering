#include <lua.hpp>
#include <raylib.h>
#include <tinyfiledialogs.h>
#include <iostream>

namespace Dithering
{
	using byte = unsigned char;

	static Image* img;
	static lua_State* L;

	static bool CheckLuaError(int code)
	{
		if(code != LUA_OK)
		{
			tinyfd_messageBox("Lua error", lua_tostring(L, -1), "ok", "error", 1);
			return false;
		}
		return true;
	}

	// Lua functions are designed to never throw errors
	// When one recieves a unexpected input it will not do anything

	static int lua_GetColor(lua_State *L)
	{
		int x = -1, y = -1;

		if (lua_isnumber(L, 1))
			x = (int)lua_tonumber(L, 1);
		if(lua_isnumber(L, 2))
			y = (int)lua_tonumber(L, 2);

		// When invalid coordinates are given it will return (0, 0, 0)
		Color c = GetImageColor(*img, x, y);

		lua_newtable(L);

		lua_pushstring(L, "r");
		lua_pushnumber(L, c.r);
		lua_settable(L, -3);

		lua_pushstring(L, "g");
		lua_pushnumber(L, c.g);
		lua_settable(L, -3);

		lua_pushstring(L, "b");
		lua_pushnumber(L, c.b);
		lua_settable(L, -3);

		return 1;
	}

	static int lua_SetColor(lua_State *L)
	{
		int x = -1, y = -1;

		if (lua_isnumber(L, 1))
			x = (int)lua_tonumber(L, 1);
		if (lua_isnumber(L, 2))
			y = (int)lua_tonumber(L, 2);

		// Don't draw to pixels out of image bounds
		if(x < 0 || x >= img->width || y < 0 || y >= img->height)
			return 0;

		if(lua_istable(L, 3))
		{
			// If some channel value is not passed then that chanel will retain the original value

			Color c = GetImageColor(*img, x, y);

			lua_pushstring(L, "r");
			lua_gettable(L, -2);
			if (lua_isnumber(L, -1))
				c.r = (byte)lua_tonumber(L, -1);
			lua_pop(L, 1);

			lua_pushstring(L, "g");
			lua_gettable(L, -2);
			if(lua_isnumber(L, -1))
				c.g = (byte)lua_tonumber(L, -1);
			lua_pop(L, 1);

			lua_pushstring(L, "b");
			lua_gettable(L, -2);
			if (lua_isnumber(L, -1))
				c.b = (byte)lua_tonumber(L, -1);
			lua_pop(L, 1);

			ImageDrawPixel(img, x, y, c);
		}
		return 0;
	}

	static int lua_GetImageSize(lua_State *L)
	{
		lua_newtable(L);

		lua_pushstring(L, "w");
		lua_pushnumber(L, img->width);
		lua_settable(L, -3);

		lua_pushstring(L, "h");
		lua_pushnumber(L, img->height);
		lua_settable(L, -3);
		return 1;
	}

	static int lua_DesaturateImage(lua_State *L)
	{
		ImageColorGrayscale(img);
		return 0;
	}

	void LuaInit(Image &image)
	{
		img = &image;
		// Open a new lua state and give acces to the basic libs
		L = luaL_newstate();
		luaL_openlibs(L);

		// Register custom functions
		lua_register(L, "GetColor", lua_GetColor);
		lua_register(L, "SetColor", lua_SetColor);
		lua_register(L, "GetImageSize", lua_GetImageSize);
		lua_register(L, "DesaturateImage", lua_DesaturateImage);
	}

	void LuaExecute(const char *scriptPath)
	{
		// Check for errors in the script file
		if(CheckLuaError(luaL_dofile(L, scriptPath)))
		{
			// Attempt to get a execute function
			lua_getglobal(L, "Execute");
			// Call it if the function was found
			if(lua_isfunction(L, -1))
				CheckLuaError(lua_pcall(L, 0, 0, 0));
			else
				tinyfd_messageBox("Lua error", "Script has to define a function 'Execute'", "ok", "error", 1);
		}
		// Close lua
		lua_close(L);
	}
}