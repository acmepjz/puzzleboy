#pragma once

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

inline SDL_Color SDL_MakeColor(Uint8 r,Uint8 g,Uint8 b,Uint8 a){
	SDL_Color clr={r,g,b,a};
	return clr;
}
