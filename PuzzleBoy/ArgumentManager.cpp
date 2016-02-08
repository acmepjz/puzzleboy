#include "ArgumentManager.h"
#include "FileSystem.h"
#include "PuzzleBoyApp.h"
#include "RandomMapScreen.h"
#include <stdio.h>
#include <string.h>
#include <SDL.h>

ArgumentManager::ArgumentManager(int argc, char** argv)
	: err(false)
	, headless(false)
	, ret(0)
	, windowFlags(0)
	, screenWidth(0)
	, screenHeight(0)
{
	int tmp;

	for (int i = 1; i<argc; i++){
		int i0 = i;

		if (strcmp(argv[i], "--help") == 0){
			printf(
				"%s: Yet another puzzle boy\n"
				"(a.k.a Kwirk <https://en.wikipedia.org/wiki/Kwirk>) clone.\n"
				"Website: https://github.com/acmepjz/puzzleboy" "\n\n"
				"Usage: %s [options]\n\n"
				"Available options:\n"
				"  --help               Display this help message.\n"
				"  -w, --width <n>      Set window width.\n"
				"  -h, --height <n>     Set window height.\n"
				"  -f, --fullscreen     Run the game fullscreen.\n"
				"  --data-dir <dir>     Specifies the data directory\n"
				"                       (will change the working directory).\n"
				"  --user-dir <dir>     Specifies the user preferences directory.\n"
				"  --database <file>    Specifies the level database.\n"
				"  --list-random-types  List predefined random map types.\n"
				"  --headless           Headless mode.\n"
				"\nHeadless mode operations:\n"
				"  -i, --input <file>   Load a level file.\n"
				"  -o, --output <file>  Save a level file.\n"
				"  --random (random|<type>|<w>,<h>,<player count>,<block type>) <level count>\n"
				"                       Generate random levels.\n"
				, argv[0], argv[0]);
			err = true; return;
		} else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--width") == 0){
			i++;
			if (i < argc && sscanf(argv[i], "%d", &tmp) == 1) screenWidth = tmp;
		} else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--height") == 0){
			i++;
			if (i < argc && sscanf(argv[i], "%d", &tmp) == 1) screenHeight = tmp;
		} else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--fullscreen") == 0){
			windowFlags |= SDL_WINDOW_FULLSCREEN;
		} else if (strcmp(argv[i], "--data-dir") == 0){
			i++;
			if (i < argc) setDataDirectory(argv[i]);
		} else if (strcmp(argv[i], "--user-dir") == 0){
			i++;
			if (i < argc) externalStoragePath = argv[i];
		} else if (strcmp(argv[i], "--database") == 0){
			i++;
			if (i < argc) levelDatabaseFileName = argv[i];
		} else if (strcmp(argv[i], "--list-random-types") == 0){
			RandomMapScreen::HeadlessListRandomTypes();
			err = true; return;
		} else if (strcmp(argv[i], "--headless") == 0){
			headless = true;
		} else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--input") == 0){
			i++;
			if (i < argc) {
				operations.push_back("-i");
				operations.push_back(argv[i]);
			}
		} else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0){
			i++;
			if (i < argc) {
				operations.push_back("-o");
				operations.push_back(argv[i]);
			}
		} else if (strcmp(argv[i], "--random") == 0){
			i+=2;
			if (i < argc) {
				operations.push_back("--random");
				operations.push_back(argv[i - 1]);
				operations.push_back(argv[i]);
			}
		} else {
			printf("Unrecognized option '%s'. Type '%s --help' for help.\n", argv[i], argv[0]);
			err = true; ret = 1; return;
		}

		if (i >= argc){
			printf("Missing argument for '%s'. Type '%s --help' for help.\n", argv[i0], argv[0]);
			err = true; ret = 1; return;
		}
	}
}

void ArgumentManager::doHeadless() {
	for (int i = 0, m = operations.size(); i < m; i++) {
		const u8string& type = operations[i];
		if (type == "-i") {
			i++;
			if (i < m) {
				printf("[doHeadless] Loading file '%s'\n", operations[i].c_str());
				theApp->LoadFile(operations[i]);
			}
		} else if (type == "-o") {
			i++;
			if (i < m) {
				printf("[doHeadless] Saving file '%s'\n", operations[i].c_str());
				theApp->SaveFile(operations[i]);
			}
		} else if (type == "--random") {
			i += 2;
			if (i < m) {
				const u8string& s1 = operations[i - 1];
				const u8string& s2 = operations[i];

				RandomMapSizeData size = {};
				int count = 0;
				int tmp1;

				if (s1 == "random") {
					//do nothing
				} else if (s1.find(',') == s1.npos) {
					if (sscanf(s1.c_str(), "%d", &tmp1) == 1) {
						size.height = tmp1;
					} else {
						i = m;
					}
				} else {
					int tmp2,tmp3,tmp4;
					if (sscanf(s1.c_str(), "%d,%d,%d,%d", &tmp1, &tmp2, &tmp3, &tmp4) == 4) {
						size.width = tmp1;
						size.height = tmp2;
						size.playerCount = tmp3;
						size.boxType = tmp4;
					} else {
						i = m;
					}
				}

				if (sscanf(s2.c_str(), "%d", &tmp1) == 1 && tmp1 > 0) {
					count = tmp1;
				} else {
					i = m;
				}

				if (i < m) {
					PuzzleBoyLevelFile *doc = RandomMapScreen::DoRandomLevelsIndirect(size, count, true);
					if (doc){
						delete theApp->m_pDocument;
						theApp->m_pDocument = doc;
						theApp->m_nCurrentLevel = 0;
					}
				}
			}
		} else {
			printf("[doHeadless] Unrecognized operation '%s'\n", type.c_str());
			ret = 1; return;
		}

		if (i >= m) {
			printf("[doHeadless] Missing argument for operation '%s'\n", type.c_str());
			ret = 1; return;
		}
	}

	printf("[doHeadless] End of headless mode\n");
}
