#include "MFCSerializer.h"
#include "FileSystem.h"
#include <assert.h>
#include <string.h>

MFCSerializer::MFCSerializer(){
	Clear();
}

void MFCSerializer::Clear(){
	m_bIsStoring=true;
	m_bIsError=false;
	m_bData.clear();
	m_objMapWrite.clear();
	m_objMapRead.clear();
	version=0;
	m_nObjCount=0;
	m_nOffset=0;
	m_pFile=NULL;
	m_nFileChunkSize=0;
}

void MFCSerializer::SetData(const std::vector<unsigned char>& data,int offset){
	m_bIsStoring=false;
	m_bIsError=false;
	m_bData=data;
	m_objMapWrite.clear();
	m_objMapRead.clear();
	version=0;
	m_nObjCount=0;
	m_nOffset=offset;
	m_pFile=NULL;
	m_nFileChunkSize=0;
}

void MFCSerializer::SetData(const void* data,int size){
	m_bIsStoring=false;
	m_bIsError=false;
	m_bData.resize(size);
	memcpy(&(m_bData[0]),data,size);
	m_objMapWrite.clear();
	m_objMapRead.clear();
	version=0;
	m_nObjCount=0;
	m_nOffset=0;
	m_pFile=NULL;
	m_nFileChunkSize=0;
}

void MFCSerializer::SetFile(u8file* file,bool isStoring,int nChunkSize){
	m_bIsStoring=isStoring;
	m_bIsError=false;
	m_nOffset=0;
	m_pFile=file;
	m_nFileChunkSize=nChunkSize;

	if(nChunkSize>1){
		m_bData.resize(nChunkSize);
		GetChunk();
	}else{
		m_bData.clear();
	}
	m_objMapWrite.clear();
	m_objMapRead.clear();
	version=0;
	m_nObjCount=0;
}

void MFCSerializer::GetChunk(){
	if(!m_bIsStoring && m_pFile!=NULL && m_nFileChunkSize>1){
		int ret=u8fread(&(m_bData[0]),1,m_nFileChunkSize,m_pFile);
		if(ret<m_nFileChunkSize) memset(&(m_bData[ret]),0,m_nFileChunkSize-ret);
	}
	m_nOffset=0;
}

void MFCSerializer::FileFlush(){
	if(m_bIsStoring){
		if(m_pFile!=NULL && m_nFileChunkSize>1 && m_nOffset>0){
			u8fwrite(&(m_bData[0]),1,m_nOffset,m_pFile);
			m_nOffset=0;
		}
	}
}

void MFCSerializer::PutInt8(unsigned char n){
	if(!m_bIsStoring) return;

	if(m_pFile){
		if(m_nFileChunkSize>1){
			m_bData[m_nOffset++]=n;
			if(m_nOffset>=m_nFileChunkSize){
				u8fwrite(&(m_bData[0]),1,m_nFileChunkSize,m_pFile);
				m_nOffset=0;
			}
		}else{
			u8fwrite(&n,1,1,m_pFile);
		}
	}else{
		m_bData.push_back(n);
	}
}

void MFCSerializer::WriteFile(u8file* file) const{
	if(m_bData.size()>0){
		u8fwrite(&(m_bData[0]),1,m_bData.size(),file);
	}
}

void MFCSerializer::PutInt16(unsigned short n){
	PutInt8(n>>0);
	PutInt8(n>>8);
}

void MFCSerializer::PutInt32(unsigned int n){
	PutInt8(n>>0);
	PutInt8(n>>8);
	PutInt8(n>>16);
	PutInt8(n>>24);
}

void MFCSerializer::PutVUInt32StringLength(bool isUnicode,unsigned int n){
	if(isUnicode){
		PutInt8(0xFF);
		PutInt16(0xFFFE);
	}

	if(n<0xFF){
		PutInt8(n);
	}else if(n<0xFFFE){
		PutInt8(0xFF);
		PutInt16(n);
	}else{
		//bigger than 4GB unsupported
		PutInt8(0xFF);
		PutInt16(0xFFFF);
		PutInt32(n);
	}
}

unsigned int MFCSerializer::GetVUInt32StringLength(bool& isUnicode){
	isUnicode=false;

	unsigned int n=GetInt8();
	if(n<0xFF) return n;

	n=GetInt16();
	if(n==0xFFFE){
		isUnicode=true;

		n=GetInt8();
		if(n<0xFF) return n;

		n=GetInt16();
	}

	if(n<0xFFFF) return n;

	//bigger than 4GB unsupported
	return GetInt32();
}

void MFCSerializer::PutVUInt32(unsigned int n){
	if(n<0xFFFF) PutInt16(n);
	else{
		//bigger than 4GB unsupported
		PutInt16(0xFFFF);
		PutInt32(n);
	}
}

unsigned int MFCSerializer::GetVUInt32(){
	unsigned int n=GetInt16();
	if(n<0xFFFF) return n;

	//bigger than 4GB unsupported
	return GetInt32();
}

int MFCSerializer::PutClass(const u8string& name,unsigned short ver){
	int idx=0;
	this->version=ver;

	MFCObjectVersion t(name,ver);
	std::map<MFCObjectVersion,int>::iterator it=m_objMapWrite.find(t);

	if(it==m_objMapWrite.end()){
		//new class
		m_objMapWrite[t]=(idx=(++m_nObjCount));

		PutInt16(0xFFFF);
		PutInt16(ver);
		int m=name.size();
		PutInt16(m);
		for(int i=0;i<m;i++) PutInt8(name[i]);
	}else{
		idx=it->second;
		if(idx<0x7FFF){
			PutInt16(idx | 0x8000);
		}else{
			PutInt16(0x7FFF);
			PutInt32(idx | 0x80000000);
		}
	}

	m_nObjCount++;
	return idx;
}

void MFCSerializer::PutClass(int idx){
	if(idx<0x7FFF){
		PutInt16(idx | 0x8000);
	}else{
		PutInt16(0x7FFF);
		PutInt32(idx | 0x80000000);
	}
}

int MFCSerializer::GetClass(u8string& name){
	name.clear();

	int n=(int)(short)GetInt16();
	if(n==0x7FFF) n=GetInt32();
	else if(n!=-1) n&=0x80007FFF;

	if(n>=0){
		m_bIsError=true;
		return -1;
	}

	if(n==-1){
		int ver=GetInt16();
		n=GetInt16();
		name.reserve(n);

		for(int i=0;i<n;i++) name.push_back(GetInt8());

		m_objMapRead[++m_nObjCount]=MFCObjectVersion(name,ver);
		m_nObjCount++;

		return (this->version=ver);
	}else{
		std::map<int,MFCObjectVersion>::iterator it=m_objMapRead.find(n & 0x7FFFFFFF);
		if(it==m_objMapRead.end()){
			m_bIsError=true;
			return -1;
		}else{
			m_nObjCount++;
			name=it->second.first;
			return (this->version=it->second.second);
		}
	}
}

u16string MFCSerializer::GetU16String(){
	u16string s;
	bool b;
	int m=GetVUInt32StringLength(b);
	if(!b){
		m_bIsError=true;
		return s;
	}

	s.reserve(m);
	for(int i=0;i<m;i++){
		s.push_back(GetInt16());
	}
	return s;
}

u8string MFCSerializer::GetU8String(){
	u8string s;
	bool b;
	int m=GetVUInt32StringLength(b);
	if(b){
		m_bIsError=true;
		return s;
	}

	s.reserve(m);
	for(int i=0;i<m;i++){
		s.push_back(GetInt8());
	}
	return s;
}

void MFCSerializer::PutU16String(const u16string& s){
	int m=s.size();
	PutVUInt32StringLength(true,m);
	for(int i=0;i<m;i++){
		PutInt16(s[i]);
	}
}

void MFCSerializer::PutU8String(const u8string& s){
	int m=s.size();
	PutVUInt32StringLength(false,m);
	for(int i=0;i<m;i++){
		PutInt8(s[i]);
	}
}

unsigned char MFCSerializer::GetInt8(){
	if(m_bIsStoring) return 0;

	if(m_pFile){
		if(m_nFileChunkSize>1){
			if(m_nOffset>=m_nFileChunkSize) GetChunk();
			return m_bData[m_nOffset++];
		}else{
			unsigned char ret=0;
			u8fread(&ret,1,1,m_pFile);
			return ret;
		}
	}else if(m_nOffset<(int)m_bData.size()){
		return m_bData[m_nOffset++];
	}

	return 0;
}

unsigned short MFCSerializer::GetInt16(){
	unsigned short a=GetInt8();
	unsigned short b=GetInt8();
	return a | (b<<8);
}

unsigned int MFCSerializer::GetInt32(){
	unsigned int a=GetInt8();
	unsigned int b=GetInt8();
	unsigned int c=GetInt8();
	unsigned int d=GetInt8();
	return a | (b<<8) | (c<<16) | (d<<24);
}
