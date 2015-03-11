#include "SimpleText.h"

#include "include_gl.h"

SimpleBaseFont::SimpleBaseFont()
:m_Tex(0)
,m_bDrawing(false)
,fontAscender(0.0f)
,fontDescender(32.0f)
,fontHeight(32.0f)
,fontAdvance(16.0f)
{
}

SimpleBaseFont::~SimpleBaseFont(){
}

void SimpleBaseFont::Destroy(){
	EndDraw();

	if(m_Tex){
		glDeleteTextures(1,&m_Tex);
		m_Tex=0;
	}

	fontAscender=0.0f;
	fontDescender=32.0f;
	fontHeight=32.0f;
	fontAdvance=16.0f;
}

void SimpleBaseFont::Reset(){
}

void SimpleBaseFont::UpdateTexture(){
}

bool SimpleBaseFont::AddChar(int c){
	return false;
}

void SimpleBaseFont::BeginDraw(){
	if(!m_bDrawing){
		UpdateTexture();

		glDisable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, m_Tex);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		m_bDrawing=true;
	}
}

void SimpleBaseFont::DrawString(const u8string& str,float x,float y,float w,float h,float scale,int flags,SDL_Color color){
	if(str.empty()) return;

	//generate vertices
	SimpleText data;
	data.AddString(this,str,x,y,w,h,scale,flags);
	if(data.empty()) return;

	//draw
	bool b=m_bDrawing;

	if(!b) BeginDraw();
	data.Draw(color);
	if(!b) EndDraw();
}

void SimpleBaseFont::DrawString(const SimpleText& str,SDL_Color color,int start,int count){
	if(str.empty()) return;

	//draw
	bool b=m_bDrawing;

	if(!b) BeginDraw();
	str.Draw(color,start,count);
	if(!b) EndDraw();
}

void SimpleBaseFont::EndDraw(){
	if(m_bDrawing){
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);

		m_bDrawing=false;
	}
}

bool SimpleBaseFont::GetCharMetric(int c,SimpleFontGlyphMetric& metric){
	return false;
}

SimpleText::SimpleText()
:xx(0),yy(0),ww(0)
,nCount(0),nRowStartIndex(0)
{
}

void SimpleText::clear(){
	xx=0;
	yy=0;
	ww=0;
	nCount=0;
	nRowStartIndex=0;

	v.clear();
	idx.clear();
	stringIdx.clear();
}

//TODO: word wrap
void SimpleText::AddString(SimpleBaseFont *font,const u8string& str,float x,float y,float w,float h,float scale,int flags){
	int o1=nCount;
	xx=0;
	yy=0;
	ww=0;

	//generate vertices
	size_t m=str.size();

	U8STRING_FOR_EACH_CHARACTER_DO_BEGIN(str,i,m,c,'?');

	AddChar(font,c,x,w,scale,flags);

	U8STRING_FOR_EACH_CHARACTER_DO_END();

	//calc size and alignment
	AddChar(font,-1,x,w,scale,flags);
	AdjustVerticalPosition(font,o1,y,h,scale,flags);
}

void SimpleText::AddChar(SimpleBaseFont *font,int c,float x,float w,float scale,int flags){
	bool bNewLine=false;
	float yNewLine=0.0f;

	SimpleFontGlyphMetric metric;

	//tell the font to load this character
	if(c>=0 && c<0x110000) font->AddChar(c);

	//add character
	switch(c){
	case -1:
		bNewLine=true;
		break;
	case ' ':
		if(font==NULL){
			xx+=scale*16.0f;
		}else if(!(font->GetCharMetric(c,metric))){
			xx+=scale*font->GetFontAdvance();
		}else{
			xx+=scale*metric.advanceX;
		}
		break;
	case 0x3000:
		if(font==NULL){
			xx+=scale*32.0f;
		}else if(!(font->GetCharMetric(c,metric))){
			xx+=scale*font->GetFontAdvance();
		}else{
			xx+=scale*metric.advanceX*2.0f;
		}
		break;
	case '\t':
		if(font==NULL){
			xx+=scale*64.0f;
		}else{
			xx+=scale*font->GetFontAdvance()*4.0f;
		}
		//xx=(xx&(-4))+4;
		break;
	case '\b':
		if(font==NULL){
			xx-=scale*16.0f;
		}else{
			xx-=scale*font->GetFontAdvance();
		}
		//if(xx>0) xx--;
		break;
	case '\n':
		if(flags & DrawTextFlags::Multiline){
			if(font==NULL){
				yNewLine=scale*32.0f;
			}else{
				yNewLine=scale*font->GetFontHeight();
			}
		}
		//fall-through
	case '\r':
		if(flags & DrawTextFlags::Multiline) bNewLine=true;
		break;
	default:
		if(font==NULL || !(font->GetCharMetric(c,metric))){
			//assume bitmap font
			if(c<32 || c>=128) c=127;

			float size=32.0f;
			if(font) size=font->GetFontHeight();
			size*=scale;

			float tx=0.0625f*(c&0xF);
			float ty=0.125f*((c>>4)-2);

			float v2[16]={
				xx,yy,tx,ty,
				size*0.5f+xx,yy,tx+0.0625f,ty,
				size*0.5f+xx,size+yy,tx+0.0625f,ty+0.125f,
				xx,size+yy,tx,ty+0.125f,
			};

			unsigned short i2[6]={
				nCount,nCount+1,nCount+2,
				nCount,nCount+2,nCount+3,
			};

			v.insert(v.end(),v2,v2+16);
			idx.insert(idx.end(),i2,i2+6);

			nCount+=4;
			xx+=size*0.5f;
		}else{
			float x1=xx+metric.destX*scale;
			float x2=x1+metric.destWidth*scale;
			float y1=yy+metric.destY*scale;
			float y2=y1+metric.destHeight*scale;

			float v2[16]={
				x1,y1,metric.srcX,metric.srcY,
				x2,y1,metric.srcX+metric.srcWidth,metric.srcY,
				x2,y2,metric.srcX+metric.srcWidth,metric.srcY+metric.srcHeight,
				x1,y2,metric.srcX,metric.srcY+metric.srcHeight,
			};

			unsigned short i2[6]={
				nCount,nCount+1,nCount+2,
				nCount,nCount+2,nCount+3,
			};

			v.insert(v.end(),v2,v2+16);
			idx.insert(idx.end(),i2,i2+6);

			nCount+=4;
			xx+=metric.advanceX*scale;
		}
		break;
	}

	//calc width
	if(xx>ww) ww=xx;

	//adjust horizontal position
	if(bNewLine){
		if(ww>w && (flags & DrawTextFlags::AutoSize)!=0){
			float factor=w/ww;

			float vcenter=yy;

			if(font) vcenter+=scale*(font->GetFontDescender()-font->GetFontAscender())*0.5f;
			else vcenter+=scale*16.0f;

			for(int i=nRowStartIndex;i<nCount;i++){
				v[i*4]*=factor;
				v[i*4+1]=vcenter+(v[i*4+1]-vcenter)*factor;
			}

			yNewLine*=factor;
		}else if(flags & DrawTextFlags::Center){
			x+=(w-ww)*0.5f;
		}else if(flags & DrawTextFlags::Right){
			x+=(w-ww);
		}

		yy+=yNewLine;

		for(int i=nRowStartIndex;i<nCount;i++){
			v[i*4]+=x;
		}

		nRowStartIndex=nCount;
		xx=0;
		ww=0;
	}
}

void SimpleText::AdjustVerticalPosition(SimpleBaseFont *font,int start,float y,float h,float scale,int flags){
	if(font==NULL){
		if(flags & DrawTextFlags::VCenter){
			y+=(h-(yy+scale*32.0f))*0.5f;
		}else if(flags & DrawTextFlags::Bottom){
			y+=(h-(yy+scale*32.0f));
		}
	}else{
		if(flags & DrawTextFlags::VCenter){
			y+=(h-(yy+scale*(font->GetFontDescender()-font->GetFontAscender())))*0.5f;
		}else if(flags & DrawTextFlags::Bottom){
			y+=(h-(yy+scale*font->GetFontDescender()));
		}else{
			y+=scale*font->GetFontAscender();
		}
	}

	for(int i=start;i<nCount;i++){
		v[i*4+1]+=y;
	}
}

void SimpleText::Draw(SDL_Color color,int start,int count) const{
	if(count==0) return;

	int st=0;
	int sz=idx.size();

	if(start>=0 && start<(int)stringIdx.size()){
		st=stringIdx[start];
		sz=((count<0 || start+count>=(int)stringIdx.size())?
			idx.size():stringIdx[start+count])-st;
	}

	glColor4ub(color.r,color.g,color.b,color.a);

	int base=(st/3)&(-0x08000);

	while(sz>0){
		int sz2=base*3+0x18000-st;
		if(sz2>sz) sz2=sz;

		glVertexPointer(2,GL_FLOAT,4*sizeof(float),&(v[base*8]));
		glTexCoordPointer(2,GL_FLOAT,4*sizeof(float),&(v[base*8+2]));
		glDrawElements(GL_TRIANGLES,sz2,GL_UNSIGNED_SHORT,&(idx[st]));

		base+=0x08000;
		sz-=sz2;
		st+=sz2;
	}
}

int SimpleText::NewStringIndex(){
	//add string to array
	stringIdx.push_back(idx.size());
	return stringIdx.size()-1;
}
