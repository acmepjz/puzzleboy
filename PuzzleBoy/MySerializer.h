#pragma once

#include "UTF8-16.h"
#include <vector>

struct u8file;

class MySerializer{
public:
	MySerializer();

	bool IsStoring() const{
		return m_bIsStoring;
	}

	void Clear();

	void PutInt8(unsigned char n);
	void PutInt16(unsigned short n);
	void PutInt32(unsigned int n);
	void PutVUInt16(unsigned short n);
	void PutVUInt32(unsigned int n);
	void PutVSInt16(short n);
	void PutVSInt32(int n);

	//UTF-16 LE
	void PutU16String(const u16string& s);

	//UTF-8
	void PutU8String(const u8string& s);

	void PutIntN(unsigned int n,int bits);
	void PutFlush();

	unsigned char GetInt8();
	unsigned short GetInt16();
	unsigned int GetInt32();
	unsigned short GetVUInt16();
	unsigned int GetVUInt32();
	short GetVSInt16();
	int GetVSInt32();

	//UTF-16 LE
	u16string GetU16String();

	//UTF-8
	u8string GetU8String();

	unsigned int GetIntN(int bits);
	void GetFlush();

	std::vector<unsigned char>& GetData(){
		return m_bData;
	}

	std::vector<unsigned char>* operator->(){
		return &m_bData;
	}

	unsigned char& operator[](int idx){
		return m_bData[idx];
	}

	int GetOffset() const{
		return m_nOffset;
	}
	void SetOffset(int offset){
		m_nOffset=offset;
	}
	void SkipOffset(int count){
		m_nOffset+=count;
	}

	void SetData(const std::vector<unsigned char>& data,int offset=0);
	void SetData(const void* data,int size);
	void SetFile(u8file* file,bool isStoring,int nChunkSize=4096);

	void FileFlush();

	void WriteFile(u8file* file) const;

	bool CalculateBlake2s(void* out,int outlen);
private:
	void GetChunk();

	bool m_bIsStoring;

	std::vector<unsigned char> m_bData;
	int m_nOffset;

	u8file* m_pFile;
	int m_nFileChunkSize;

	unsigned int m_nUnsaved;
	int m_nUnsavedSize;
};
