#pragma once

#include <vector>
#include "UTF8-16.h"

class ArgumentManager
{
public:
	ArgumentManager(int argc, char** argv);
	void doHeadless();
public:
	bool err;
	bool headless;
	int ret;
	int windowFlags;
	int screenWidth;
	int screenHeight;
	u8string levelDatabaseFileName;

	std::vector<u8string> operations;
};

