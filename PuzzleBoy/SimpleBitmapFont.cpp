#include "SimpleBitmapFont.h"
#include <vector>

#include "include_gl.h"

SimpleBitmapFont::SimpleBitmapFont(SDL_Surface* surf){
	//FIXME: Android doesn't support GL_BGRA
	int internalformat,format;
	switch(surf->format->BitsPerPixel){
	case 32:
		internalformat=GL_RGBA;
		format=GL_RGBA;
#ifndef ANDROID
		if(surf->format->Rmask!=0xFF) format=GL_BGRA;
#endif
		break;
	case 24:
		internalformat=GL_RGB;
		format=GL_RGB;
#ifndef ANDROID
		if(surf->format->Rmask!=0xFF) format=GL_BGR;
#endif
		break;
	case 8: //only alpha channel
		internalformat=GL_ALPHA;
		format=GL_ALPHA;
		break;
	default: //unsupported
		return;
	}

	glGenTextures(1,&m_Tex);
	glBindTexture(GL_TEXTURE_2D, m_Tex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifndef ANDROID
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
#endif

	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, surf->w, surf->h, 0, format, GL_UNSIGNED_BYTE, surf->pixels);

	glBindTexture(GL_TEXTURE_2D, 0);
}

SimpleBitmapFont::~SimpleBitmapFont(){
	Destroy();
}
