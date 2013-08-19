#pragma once

#include "Level.h"
#include <vector>
#include <stdio.h>

struct RecordItem{
	CString sPlayerName;
	int nStep;
	std::vector<unsigned char> bSolution;
};

struct RecordLevelChecksum{
	unsigned char bChecksum[CLevel::ChecksumSize];
	bool operator==(const RecordLevelChecksum& other) const;
	bool operator!=(const RecordLevelChecksum& other) const;
	bool operator>(const RecordLevelChecksum& other) const;
	bool operator<(const RecordLevelChecksum& other) const;
	bool operator>=(const RecordLevelChecksum& other) const;
	bool operator<=(const RecordLevelChecksum& other) const;
	unsigned char& operator[](int idx){
		return bChecksum[idx];
	}
	const unsigned char& operator[](int idx) const{
		return bChecksum[idx];
	}
};

struct RecordLevelItem{
	RecordLevelChecksum objChecksum;
	int nWidth;
	int nHeight;
	int nBestRecord;
	int nUserData;
	CString sName;
	std::vector<unsigned char> bData;
	std::vector<RecordItem> objRecord;
};

struct RecordFile{
	std::vector<RecordLevelItem> objLevels;
};

class RecordManager{
public:
	static const int HashTableBits=10;

	RecordManager();
	~RecordManager();

	bool LoadFile(const char* fileName=NULL);
	void CloseFile();

	static bool LoadWholeFile(FILE* f,RecordFile& ret);
	bool LoadWholeFileAndClose(RecordFile& ret);

	static bool SaveWholeFile(FILE* f,RecordFile& ret);
	bool SaveWholeFile(RecordFile& ret,const char* fileName=NULL);

	int AddLevel(CLevel& lev);
	void AddLevelAndRecord(CLevel& lev,int nStep,const CString& rec,const wchar_t* playerName);

	//return value: steps, -1=not found, -2=level unchanged
	int FindLevelAndRecord(CLevel& lev,const wchar_t* playerName,CString* rec,unsigned char* lastChecksum=NULL,CString* returnName=NULL);

	static void ConvertRecordDataToString(std::vector<unsigned char>& bSolution,CString& rec);

	//return value: best steps, -1=not found, -2=level unchanged
	int FindAllRecordsOfLevel(CLevel& lev,std::vector<RecordItem>& ret,unsigned char* lastChecksum=NULL);

	int GetLevelCount();
private:
	static void ConvertRecordDataToString(MySerializer& ar3,int sz,CString& rec);
private:
	/*void LoadStrings();
	void LoadUnusedBlocks();*/
	int SerializeAndFindLevel(CLevel& lev,MySerializer& ar,int& idx,unsigned char* bChecksum);
	int AllocateAndMoveToBlock(int nSize);
private:
	char* m_sFileName;
	FILE* m_pFile;

	//header data
	int m_nHeader[(1UL<<HashTableBits)+8];
	//strings
	CArray<CString> m_Strings;
	int m_lpLastString;

	/*CArray<int> m_nUnusedBlockOffset;
	CArray<int> m_nUnusedBlockSize;*/
};
