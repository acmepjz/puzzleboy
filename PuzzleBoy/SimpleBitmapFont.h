#pragma once

#include "SimpleText.h"

class SimpleBitmapFont:public SimpleBaseFont{
public:
	SimpleBitmapFont(SDL_Surface* surf);
	~SimpleBitmapFont();
};
