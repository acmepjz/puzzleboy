#pragma once

#ifdef ANDROID
#include <SDL_opengles.h>
#define glFrustum glFrustumf
#define glOrtho glOrthof
#else
#include <SDL_opengl.h>
#endif
