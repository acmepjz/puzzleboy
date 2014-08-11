#pragma once

#include "PuzzleBoyLevelData.h"
#include "MySerializer.h"
#include "UTF8-16.h"
#include <vector>

struct u8file;

struct RecordItem{
	u16string sPlayerName;
	int nStep;
	std::vector<unsigned char> bSolution;
};

struct RecordLevelChecksum{
	unsigned char bChecksum[PuzzleBoyLevelData::ChecksumSize];
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
	u16string sName;
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

	static bool LoadWholeFile(u8file* f,RecordFile& ret);
	bool LoadWholeFileAndClose(RecordFile& ret);

	static bool SaveWholeFile(u8file* f,RecordFile& ret);
	bool SaveWholeFile(RecordFile& ret,const char* fileName=NULL);

	int AddLevel(const PuzzleBoyLevelData& lev);
	void AddLevelAndRecord(const PuzzleBoyLevelData& lev,int nStep,const u8string& rec,const unsigned short* playerName);

	//return value: steps, -1=not found, -2=level unchanged
	int FindLevelAndRecord(const PuzzleBoyLevelData& lev,const unsigned short* playerName,u8string* rec,unsigned char* lastChecksum=NULL,u16string* returnName=NULL);

	static void ConvertRecordDataToString(const std::vector<unsigned char>& bSolution,u8string& rec);

	//return value: best steps, -1=not found, -2=level unchanged
	int FindAllRecordsOfLevel(const PuzzleBoyLevelData& lev,std::vector<RecordItem>& ret,unsigned char* lastChecksum=NULL);

	int GetLevelCount();
private:
	static void ConvertRecordDataToString(MySerializer& ar3,int sz,u8string& rec);
private:
	/*void LoadStrings();
	void LoadUnusedBlocks();*/
	int SerializeAndFindLevel(const PuzzleBoyLevelData& lev,MySerializer& ar,int& idx,unsigned char* bChecksum);
	int AllocateAndMoveToBlock(int nSize);
private:
	char* m_sFileName;
	u8file* m_pFile;

	//header data
	int m_nHeader[(1UL<<HashTableBits)+8];
	//strings
	std::vector<u16string> m_Strings;
	int m_lpLastString;

	/*std::vector<int> m_nUnusedBlockOffset;
	std::vector<int> m_nUnusedBlockSize;*/
};
