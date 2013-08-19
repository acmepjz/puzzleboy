#pragma once

#include <stdio.h>

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
	void PutString(const CString& s);

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
	CString GetString();

	unsigned int GetIntN(int bits);
	void GetFlush();

	CArray<unsigned char>& GetData(){
		return m_bData;
	}

	CArray<unsigned char>* operator->(){
		return &m_bData;
	}

	unsigned char& operator[](int idx){
		return m_bData[idx];
	}

	void SetData(const CArray<unsigned char>& data,int offset=0);
	void SetData(const void* data,int size);
	void SetFile(FILE* file,bool isStoring,int nChunkSize=4096);

	void FileFlush();

	void WriteFile(FILE* file) const;

	bool CalculateBlake2s(void* out,int outlen);
private:
	void GetChunk();

	bool m_bIsStoring;

	CArray<unsigned char> m_bData;
	int m_nOffset;

	FILE* m_pFile;
	int m_nFileChunkSize;

	unsigned int m_nUnsaved;
	int m_nUnsavedSize;
};
