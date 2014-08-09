#include "SimpleBitmapFont.h"
#include "main.h"
#include <vector>

#include "include_gl.h"

SimpleBitmapFont::SimpleBitmapFont(SDL_Surface* surf){
	m_Tex=CreateGLTexture(0,0,GL_ALPHA,GL_CLAMP_TO_EDGE,GL_LINEAR,GL_LINEAR,NULL,surf,NULL);
}

SimpleBitmapFont::~SimpleBitmapFont(){
	Destroy();
}
