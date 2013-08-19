#include "stdafx.h"
#include "MySerializer.h"
#include "blake2.h"
#include <assert.h>
#include <string.h>

MySerializer::MySerializer(){
	Clear();
}

void MySerializer::Clear(){
	m_bIsStoring=true;
	m_bData.RemoveAll();
	m_nUnsaved=0;
	m_nUnsavedSize=0;
	m_nOffset=0;
	m_pFile=NULL;
	m_nFileChunkSize=0;
}

void MySerializer::SetData(const CArray<unsigned char>& data,int offset){
	m_bIsStoring=false;
	m_bData.Copy(data);
	m_nUnsaved=0;
	m_nUnsavedSize=0;
	m_nOffset=offset;
	m_pFile=NULL;
	m_nFileChunkSize=0;
}

void MySerializer::SetData(const void* data,int size){
	m_bIsStoring=false;
	m_bData.SetSize(size);
	memcpy(&(m_bData[0]),data,size);
	m_nUnsaved=0;
	m_nUnsavedSize=0;
	m_nOffset=0;
	m_pFile=NULL;
	m_nFileChunkSize=0;
}

void MySerializer::SetFile(FILE* file,bool isStoring,int nChunkSize){
	m_bIsStoring=isStoring;
	m_nUnsaved=0;
	m_nUnsavedSize=0;
	m_nOffset=0;
	m_pFile=file;
	m_nFileChunkSize=nChunkSize;

	if(nChunkSize>1){
		m_bData.SetSize(nChunkSize);
		GetChunk();
	}else{
		m_bData.RemoveAll();
	}
}

void MySerializer::GetChunk(){
	if(!m_bIsStoring && m_pFile!=NULL && m_nFileChunkSize>1){
		int ret=fread(&(m_bData[0]),1,m_nFileChunkSize,m_pFile);
		if(ret<m_nFileChunkSize) memset(&(m_bData[ret]),0,m_nFileChunkSize-ret);
	}
	m_nOffset=0;
}

void MySerializer::FileFlush(){
	if(m_bIsStoring){
		PutFlush();
		if(m_pFile!=NULL && m_nFileChunkSize>1 && m_nOffset>0){
			fwrite(&(m_bData[0]),1,m_nOffset,m_pFile);
			m_nOffset=0;
		}
	}
}

void MySerializer::PutInt8(unsigned char n){
	if(!m_bIsStoring) return;

	if(m_pFile){
		if(m_nFileChunkSize>1){
			m_bData[m_nOffset++]=n;
			if(m_nOffset>=m_nFileChunkSize){
				fwrite(&(m_bData[0]),1,m_nFileChunkSize,m_pFile);
				m_nOffset=0;
			}
		}else{
			fwrite(&n,1,1,m_pFile);
		}
	}else{
		m_bData.Add(n);
	}
}

void MySerializer::WriteFile(FILE* file) const{
	if(m_bData.GetSize()>0){
		fwrite(&(m_bData[0]),1,m_bData.GetSize(),file);
	}
}

void MySerializer::PutInt16(unsigned short n){
	PutInt8(n>>0);
	PutInt8(n>>8);
}

void MySerializer::PutInt32(unsigned int n){
	PutInt8(n>>0);
	PutInt8(n>>8);
	PutInt8(n>>16);
	PutInt8(n>>24);
}

void MySerializer::PutVUInt16(unsigned short n){
	if(n<0x80){
		PutInt8(n<<1);
		return;
	}else{
		n-=0x80;
	}

	if(n<0x4000){
		PutInt8((n<<2)|0x1);
		PutInt8(n>>6);
		return;
	}else{
		n-=0x4000;
	}

	PutInt8((n<<2)|0x3);
	PutInt8(n>>6);
	PutInt8(n>>14);
}

void MySerializer::PutVUInt32(unsigned int n){
	if(n<0x80){
		PutInt8(n<<1);
		return;
	}else{
		n-=0x80;
	}

	if(n<0x2000){
		PutInt8((n<<3)|0x1);
		PutInt8(n>>5);
		return;
	}else{
		n-=0x2000;
	}

	if(n<0x200000){
		PutInt8((n<<3)|0x3);
		PutInt8(n>>5);
		PutInt8(n>>13);
		return;
	}else{
		n-=0x200000;
	}

	if(n<0x20000000){
		PutInt8((n<<3)|0x5);
		PutInt8(n>>5);
		PutInt8(n>>13);
		PutInt8(n>>21);
		return;
	}else{
		n-=0x20000000;
	}

	PutInt8((n<<3)|0x7);
	PutInt8(n>>5);
	PutInt8(n>>13);
	PutInt8(n>>21);
	PutInt8(n>>29);
}

void MySerializer::PutVSInt16(short n){
	PutVUInt16(n>=0?(n<<1):(((~n)<<1)|0x1));
}

void MySerializer::PutVSInt32(int n){
	PutVUInt32(n>=0?(n<<1):(((~n)<<1)|0x1));
}

CString MySerializer::GetString(){
	CString s;
	int m=GetVUInt32();
	for(int i=0;i<m;i++){
		s.AppendChar(GetInt16());
	}
	return s;
}

void MySerializer::PutString(const CString& s){
	int m=s.GetLength();
	PutVUInt32(m);
	for(int i=0;i<m;i++){
		PutInt16(s[i]);
	}
}

void MySerializer::PutIntN(unsigned int n,int bits){
	//FIXME: only works when m_nUnsavedSize+bits<32
	m_nUnsaved|=(n&((1UL<<bits)-1))<<m_nUnsavedSize;
	m_nUnsavedSize+=bits;

	while(m_nUnsavedSize>=8){
		PutInt8(m_nUnsaved>>0);
		m_nUnsaved>>=8;
		m_nUnsavedSize-=8;
	}
}

void MySerializer::PutFlush(){
	while(m_nUnsavedSize>0){
		PutInt8(m_nUnsaved>>0);
		m_nUnsaved>>=8;
		m_nUnsavedSize-=8;
	}
	m_nUnsaved=0;
	m_nUnsavedSize=0;
}

bool MySerializer::CalculateBlake2s(void* out,int outlen){
	void* lp;
	lp=m_bData.GetSize()>0?(void*)(&(m_bData[0])):(void*)(&lp);

	if(blake2s((uint8_t*)out,lp,NULL,outlen,m_bData.GetSize(),0)<0) return false;

	return true;
}

unsigned char MySerializer::GetInt8(){
	if(m_bIsStoring) return 0;

	if(m_pFile){
		if(m_nFileChunkSize>1){
			if(m_nOffset>=m_nFileChunkSize) GetChunk();
			return m_bData[m_nOffset++];
		}else{
			unsigned char ret=0;
			fread(&ret,1,1,m_pFile);
			return ret;
		}
	}else if(m_nOffset<m_bData.GetSize()){
		return m_bData[m_nOffset++];
	}

	return 0;
}

unsigned short MySerializer::GetInt16(){
	unsigned short a=GetInt8();
	unsigned short b=GetInt8();
	return a | (b<<8);
}

unsigned int MySerializer::GetInt32(){
	unsigned int a=GetInt8();
	unsigned int b=GetInt8();
	unsigned int c=GetInt8();
	unsigned int d=GetInt8();
	return a | (b<<8) | (c<<16) | (d<<24);
}

unsigned short MySerializer::GetVUInt16(){
	unsigned short a=GetInt8();
	if(a&0x1){
		unsigned short b=GetInt8();
		if(a&0x2){
			unsigned short c=GetInt8();
			return ((a>>2) | (b<<6) | (c<<14))+0x80+0x4000;
		}else{
			return ((a>>2) | (b<<6))+0x80;
		}
	}else{
		return a>>1;
	}
}

unsigned int MySerializer::GetVUInt32(){
	unsigned int a=GetInt8();
	if(a&0x1){
		unsigned int b=GetInt8();
		unsigned int aa=a&0x7;

		if(aa>=0x3){
			unsigned int c=GetInt8();
			if(aa>=0x5){
				unsigned int d=GetInt8();
				if(aa>=0x7){
					unsigned int e=GetInt8();
					return ((a>>3) | (b<<5) | (c<<13) | (d<<21) | (e<<29))+0x80+0x2000+0x200000+0x20000000;
				}else{
					return ((a>>3) | (b<<5) | (c<<13) | (d<<21))+0x80+0x2000+0x200000;
				}
			}else{
				return ((a>>3) | (b<<5) | (c<<13))+0x80+0x2000;
			}
		}else{
			return ((a>>3) | (b<<5))+0x80;
		}
	}else{
		return a>>1;
	}
}

short MySerializer::GetVSInt16(){
	unsigned short ret=GetVUInt16();
	return (ret&0x1)?(~(ret>>1)):(ret>>1);
}

int MySerializer::GetVSInt32(){
	unsigned int ret=GetVUInt32();
	return (ret&0x1)?(~(ret>>1)):(ret>>1);
}

unsigned int MySerializer::GetIntN(int bits){
	//FIXME: only works when bits<=24
	while(m_nUnsavedSize<bits){
		m_nUnsaved|=((unsigned int)GetInt8())<<m_nUnsavedSize;
		m_nUnsavedSize+=8;
	}

	unsigned int ret=m_nUnsaved&((1UL<<bits)-1);
	m_nUnsaved>>=bits;
	m_nUnsavedSize-=bits;

	return ret;
}

void MySerializer::GetFlush(){
	m_nUnsaved=0;
	m_nUnsavedSize=0;
}
