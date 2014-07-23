#pragma once

#if defined(ANDROID) || defined(__IPHONEOS__)
#ifndef USE_OPENGLES
#define USE_OPENGLES
#endif
#endif

#ifdef USE_OPENGLES
#include <SDL_opengles.h>
#define glFrustum glFrustumf
#define glOrtho glOrthof
#else
#include <SDL_opengl.h>
#endif
