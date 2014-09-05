#pragma once

#include "UTF8-16.h"

bool SimpleInputScreen(const u8string& title,const u8string& prompt,u8string& text,const char* allowedChars=0);

//-1 means cancel
int SimpleConfigKeyScreen(int key);

