#pragma once

#define USE_SDL_RWOPS

#include "UTF8-16.h"
#include <stdio.h>
#include <vector>

#ifdef USE_SDL_RWOPS
#include <SDL_rwops.h>
typedef SDL_RWops u8file;
#define u8fopen(filename, mode) SDL_RWFromFile(filename, mode)
#define u8fseek(file, offset, whence) SDL_RWseek(file, offset, whence)
#define u8ftell(file) SDL_RWtell(file)
#define u8fread(ptr, size, n, file) SDL_RWread(file, ptr, size, n)
#define u8fwrite(ptr, size, n, file) SDL_RWwrite(file, ptr, size, n)
#define u8fclose(file) SDL_RWclose(file)
char* u8fgets(char* buf, int count, u8file* file);
#else
typedef FILE u8file;
#ifdef WIN32
u8file *u8fopen(const char* filename,const char* mode);
#else
#define u8fopen(filename, mode) fopen(filename, mode)
#endif
#define u8fseek(file, offset, whence) fseek(file, offset, whence)
#define u8ftell(file) ftell(file)
#define u8fread(ptr, size, n, file) fread(ptr, size, n, file)
#define u8fwrite(ptr, size, n, file) fwrite(ptr, size, n, file)
#define u8fclose(file) fclose(file)
#define u8fgets(buf, count, file) fgets(buf, count, file)
#endif

//Copied from Me and My Shadow, licensed under GPLv3 or above

//Method that returns a list of all the files in a given directory.
//path: The path to list the files of.
//extension: The extension the files must have.
//containsPath: Specifies if the return file name should contains path.
//Returns: A vector containing the names of the files.
std::vector<u8string> enumAllFiles(u8string path,const char* extension=NULL,bool containsPath=false);

//Method that returns a list of all the directories in a given directory.
//path: The path to list the directory of.
//containsPath: Specifies if the return file name should contains path.
//Returns: A vector containing the names of the directories.
std::vector<u8string> enumAllDirs(u8string path,bool containsPath=false);
