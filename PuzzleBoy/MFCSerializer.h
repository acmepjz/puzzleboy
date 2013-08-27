#pragma once

#include "UTF8-16.h"
#include <vector>
#include <map>

struct u8file;

typedef std::pair<u8string,unsigned short> MFCObjectVersion;

class MFCSerializer{
public:
	MFCSerializer();

	bool IsStoring() const{
		return m_bIsStoring;
	}

	bool IsError() const{
		return m_bIsError;
	}

	void Clear();

	void PutInt8(unsigned char n);
	void PutInt16(unsigned short n);
	void PutInt32(unsigned int n);

	void PutVUInt32StringLength(bool isUnicode,unsigned int n);
	void PutVUInt32(unsigned int n);
	int PutClass(const u8string& name,unsigned short version);

	int version;

	//UTF-16 LE
	void PutU16String(const u16string& s);

	//UTF-8
	void PutU8String(const u8string& s);

	unsigned char GetInt8();
	unsigned short GetInt16();
	unsigned int GetInt32();

	unsigned int GetVUInt32StringLength(bool& isUnicode);
	unsigned int GetVUInt32();
	int GetClass(u8string& name);

	//UTF-16 LE
	u16string GetU16String();

	//UTF-8
	u8string GetU8String();

	std::vector<unsigned char>& GetData(){
		return m_bData;
	}

	std::vector<unsigned char>* operator->(){
		return &m_bData;
	}

	unsigned char& operator[](int idx){
		return m_bData[idx];
	}

	void SetData(const std::vector<unsigned char>& data,int offset=0);
	void SetData(const void* data,int size);
	void SetFile(u8file* file,bool isStoring,int nChunkSize=4096);

	void FileFlush();

	void WriteFile(u8file* file) const;

	MFCSerializer& operator<<(char n){PutInt8(n);return *this;}
	MFCSerializer& operator<<(unsigned char n){PutInt8(n);return *this;}
	MFCSerializer& operator<<(short n){PutInt16(n);return *this;}
	MFCSerializer& operator<<(unsigned short n){PutInt16(n);return *this;}
	MFCSerializer& operator<<(int n){PutInt32(n);return *this;}
	MFCSerializer& operator<<(unsigned int n){PutInt32(n);return *this;}

	MFCSerializer& operator>>(char &n){n=GetInt8();return *this;}
	MFCSerializer& operator>>(unsigned char &n){n=GetInt8();return *this;}
	MFCSerializer& operator>>(short &n){n=GetInt16();return *this;}
	MFCSerializer& operator>>(unsigned short &n){n=GetInt16();return *this;}
	MFCSerializer& operator>>(int &n){n=GetInt32();return *this;}
	MFCSerializer& operator>>(unsigned int &n){n=GetInt32();return *this;}

	template<class T>
	MFCSerializer& operator<<(const std::vector<T>& v){
		unsigned int m=v.size();
		PutVUInt32(m);
		for(unsigned int i=0;i<m;i++) (*this)<<v[i];
		return *this;
	}

	template<class T>
	MFCSerializer& operator>>(std::vector<T>& v){
		unsigned int m=GetVUInt32();
		v.resize(m);
		for(unsigned int i=0;i<m;i++) (*this)>>v[i];
		return *this;
	}

	MFCSerializer& operator<<(const u16string& s){PutU16String(s);return *this;}
	MFCSerializer& operator<<(const u8string& s){PutU8String(s);return *this;}

	MFCSerializer& operator>>(u16string& s){s=GetU16String();return *this;}
	MFCSerializer& operator>>(u8string& s){s=GetU8String();return *this;}

	template<class T>
	void PutObjectArray(std::vector<T*>& v,const u8string& name,unsigned short version){
		int idx=0;
		unsigned int m=v.size();
		PutVUInt32(m);
		for(unsigned int i=0;i<m;i++){
			if(i==0) idx=PutClass(name,version);
			else PutClass(idx);
			if(!(v[i]->MFCSerialize(*this))){
				m_bIsError=true;
				return;
			}
		}
	}

	template<class T>
	bool GetObjectArray(std::vector<T*>& v,const u8string& name){
		unsigned int m=GetVUInt32();
		for(unsigned int i=0;i<m;i++){
			u8string s;
			if(GetClass(s)<0 || s!=name){
				m_bIsError=true;
				return false;
			}
			T* t=new T;
			if(!(t->MFCSerialize(*this))){
				delete t;
				m_bIsError=true;
				return false;
			}
			v.push_back(t);
		}
		return true;
	}
private:
	void GetChunk();

	//internal function
	void PutClass(int idx);

	bool m_bIsStoring,m_bIsError;

	std::vector<unsigned char> m_bData;
	int m_nOffset;

	u8file* m_pFile;
	int m_nFileChunkSize;

	std::map<MFCObjectVersion,int> m_objMapWrite;
	std::map<int,MFCObjectVersion> m_objMapRead;
	int m_nObjCount;
};
