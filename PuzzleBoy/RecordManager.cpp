#include "RecordManager.h"
#include "MySerializer.h"
#include "PushableBlock.h"
#include "FileSystem.h"
#include <SDL_stdinc.h>
#include <SDL_endian.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <map>

bool RecordLevelChecksum::operator==(const RecordLevelChecksum& other) const{
	return memcmp(bChecksum,other.bChecksum,sizeof(other))==0;
}

bool RecordLevelChecksum::operator!=(const RecordLevelChecksum& other) const{
	return memcmp(bChecksum,other.bChecksum,sizeof(other))!=0;
}

bool RecordLevelChecksum::operator>(const RecordLevelChecksum& other) const{
	return memcmp(bChecksum,other.bChecksum,sizeof(other))>0;
}

bool RecordLevelChecksum::operator<(const RecordLevelChecksum& other) const{
	return memcmp(bChecksum,other.bChecksum,sizeof(other))<0;
}

bool RecordLevelChecksum::operator>=(const RecordLevelChecksum& other) const{
	return memcmp(bChecksum,other.bChecksum,sizeof(other))>=0;
}

bool RecordLevelChecksum::operator<=(const RecordLevelChecksum& other) const{
	return memcmp(bChecksum,other.bChecksum,sizeof(other))<=0;
}

RecordManager::RecordManager(){
	m_sFileName=NULL;
	m_pFile=NULL;
	m_lpLastString=0;
}

RecordManager::~RecordManager(){
	CloseFile();

	if(m_sFileName){
		SDL_free(m_sFileName);
		m_sFileName=NULL;
	}
}

#if SDL_BYTEORDER==SDL_LIL_ENDIAN

#define SWAP_LE32(X)
#define SWAP_LE32_ARRAY(X)
#define WRITE_HEADER(X,F) u8fwrite(X,1,sizeof(X),F)

#else

#define SWAP_LE32(X) X=SDL_SwapLE32(X)
#define SWAP_LE32_ARRAY(X) {for(int i=0;i<sizeof(X)/sizeof(X[0]);i++) X[i]=SDL_SwapLE32(X[i]);}
#define WRITE_HEADER(X,F) \
{ \
	int nHeader[sizeof(X)/4]; \
	for(int i=0;i<sizeof(X)/4;i++){ \
		nHeader[i]=SDL_SwapLE32(X[i]); \
	} \
	u8fwrite(nHeader,1,sizeof(X),F); \
}

#endif

bool RecordManager::LoadWholeFile(u8file* f,RecordFile& ret){
	//some sanity check
	if(f==NULL) return false;

	char sig[16];
	int nHeader[(1UL<<HashTableBits)+8];

	//read header
	u8fseek(f,0,SEEK_SET);
	if(u8fread(sig,1,16,f)!=16 || memcmp(sig,"PuzzleBoyRecord\x1A",16)!=0){
		return false;
	}

	memset(nHeader,0,sizeof(nHeader));
	u8fread(nHeader,1,sizeof(nHeader),f);

	SWAP_LE32_ARRAY(nHeader);

	//load player name
	MySerializer ar;

	int lp=nHeader[(1<<HashTableBits)];
	ar.SetFile(f,false,0);

	std::vector<u16string> sStrings;

	while(lp!=0){
		u8fseek(f,lp,SEEK_SET);
		int lpNext=ar.GetInt32();
		sStrings.push_back(ar.GetU16String());
		lp=lpNext;
	}

	//read all level
	ret.objLevels.clear();

	for(int idx=0;idx<(1<<HashTableBits);idx++){
		lp=nHeader[idx];
		while(lp>0){
			u8fseek(f,lp,SEEK_SET);

			int lpRecord=0;
			u8fread(&lpRecord,4,1,f); //position of first (last) solution
			SWAP_LE32(lpRecord);

			lp=0;
			u8fread(&lp,4,1,f); //position of next level
			SWAP_LE32(lp);

			RecordLevelItem lev;
			lev.objChecksum[0]=idx>>0;
			u8fread(&(lev.objChecksum[1]),1,PuzzleBoyLevelData::ChecksumSize-1,f); //BLAKE2s-160 checksum

			lev.sName=ar.GetU16String(); //level name

			//parse level data
			int lpStart=u8ftell(f);
			lev.nWidth=ar.GetVUInt32();
			lev.nHeight=ar.GetVUInt32();
			lev.nBestRecord=0;
			lev.nUserData=0;

			int m=(lev.nWidth*lev.nHeight+1)>>1;
			u8fseek(f,m,SEEK_CUR);

			m=ar.GetVUInt32();
			for(int i=0;i<m;i++){
				int nType=ar.GetVUInt32();
				ar.GetVUInt32();
				ar.GetVUInt32();

				if(nType!=ROTATE_BLOCK){
					int w=ar.GetVUInt32();
					int h=ar.GetVUInt32();
					u8fseek(f,(w*h+7)>>3,SEEK_CUR);
				}else{
					u8fseek(f,4,SEEK_CUR);
				}
			}

			//calculate level data size and read
			m=u8ftell(f)-lpStart;
			if(m>0){
				lev.bData.resize(m);
				u8fseek(f,lpStart,SEEK_SET);
				u8fread(&(lev.bData[0]),1,m,f);
			}

			//read all records
			while(lpRecord>0){
				u8fseek(f,lpRecord,SEEK_SET);

				lpRecord=0;
				u8fread(&lpRecord,4,1,f); //position of next solution
				SWAP_LE32(lpRecord);

				int idxName=ar.GetVUInt32(); //index of string represents player name
				int st=ar.GetVUInt32(); //steps
				int sz=ar.GetVUInt32(); //solution length (in bytes)

				RecordItem rec;
				if(idxName>=0 && idxName<(int)sStrings.size()){
					rec.sPlayerName=sStrings[idxName];
				}
				rec.nStep=st;

				if(sz>0){
					rec.bSolution.resize(sz);
					u8fread(&(rec.bSolution[0]),1,sz,f);
				}

				//add
				if(lev.nBestRecord==0 || lev.nBestRecord>rec.nStep) lev.nBestRecord=rec.nStep;
				lev.objRecord.push_back(rec);
			}

			//add
			ret.objLevels.push_back(lev);
		}
	}

	return true;
}

bool RecordManager::LoadWholeFileAndClose(RecordFile& ret){
	bool b=LoadWholeFile(m_pFile,ret);
	CloseFile();
	return b;
}

bool RecordManager::SaveWholeFile(u8file* f,RecordFile& ret){
	//some sanity check
	if(f==NULL) return false;

	MySerializer ar,ar2;
	int nHeader[(1UL<<HashTableBits)+8];
	int nPrevious[1UL<<HashTableBits];

	std::vector<u16string> sStrings;
	std::map<u16string,int>  mapStrings;

	//clear header
	memset(nHeader,0,sizeof(nHeader));
	memset(nPrevious,0,sizeof(nPrevious));

	int lp=(16+sizeof(nHeader));
	for(unsigned int i=0;i<ret.objLevels.size();i++){
		RecordLevelItem& lev=ret.objLevels[i];

		if(lev.bData.empty()) continue;

		int idx=int(lev.objChecksum[0]) | (int(lev.objChecksum[1]&((1<<(HashTableBits-8))-1))<<8);
		int nPreviousRecord=lp;

		//link previous level to this level
		if(nPrevious[idx]>0){
			int tmp=SDL_SwapLE32(lp);
			memcpy(&(ar[nPrevious[idx]+4-(16+sizeof(nHeader))]),&tmp,4);
		}else{
			nHeader[idx]=lp;
		}

		//set current level
		nPrevious[idx]=lp;

		//create level data
		ar2.Clear();
		ar2.PutInt32(0); //position of first (last) solution
		ar2.PutInt32(0); //position of next level
		for(int k=1;k<PuzzleBoyLevelData::ChecksumSize;k++){
			ar2.PutInt8(lev.objChecksum[k]); //BLAKE2s-160 checksum
		}
		ar2.PutU16String(lev.sName); //level name

		if(lev.bData.size()>0){
			ar2->insert(ar2->end(),lev.bData.begin(),lev.bData.end());
		}

		//save level
		ar->insert(ar->end(),ar2->begin(),ar2->end());
		lp+=ar2->size();

		//save all records
		for(unsigned int j=0;j<lev.objRecord.size();j++){
			//link previous record to this record
			{
				int tmp=SDL_SwapLE32(lp);
				memcpy(&(ar[nPreviousRecord-(16+sizeof(nHeader))]),&tmp,4);
			}

			//set current record
			nPreviousRecord=lp;

			//create record data
			ar2.Clear();
			ar2.PutInt32(0); //position of next solution

			int idxName=0;
			std::map<u16string,int>::const_iterator it=mapStrings.find(lev.objRecord[j].sPlayerName);
			if(it==mapStrings.end()){
				//add new name
				idxName=sStrings.size();
				sStrings.push_back(lev.objRecord[j].sPlayerName);
				mapStrings[lev.objRecord[j].sPlayerName]=idxName;
			}else{
				idxName=it->second;
			}
			ar2.PutVUInt32(idxName); //index of string represents player name

			ar2.PutVUInt32(lev.objRecord[j].nStep); //steps

			int n=lev.objRecord[j].bSolution.size();
			ar2.PutVUInt32(n); //solution length (in bytes)
			if(n>0){
				ar2->insert(ar2->end(),
					lev.objRecord[j].bSolution.begin(),
					lev.objRecord[j].bSolution.end()
					); //solution data
			}

			//save record
			ar->insert(ar->end(),ar2->begin(),ar2->end());
			lp+=ar2->size();
		}
	}

	//save player names
	nHeader[1UL<<HashTableBits]=lp;
	for(int i=0;i<(int)sStrings.size();i++){
		ar2.Clear();
		ar2.PutInt32(0); //position of next string
		ar2.PutU16String(sStrings[i]); //string

		int lpNext=lp+ar2->size();

		if(i<(int)sStrings.size()-1){
			int tmp=SDL_SwapLE32(lpNext);
			memcpy(&(ar2[0]),&tmp,4); //position of next string
		}

		ar->insert(ar->end(),ar2->begin(),ar2->end());
		lp=lpNext;
	}

	//save header
	SWAP_LE32_ARRAY(nHeader);
	u8fwrite("PuzzleBoyRecord\x1A",1,16,f);
	u8fwrite(nHeader,1,sizeof(nHeader),f);

	//save data
	ar.WriteFile(f);

	return true;
}

bool RecordManager::SaveWholeFile(RecordFile& ret,const char* fileName){
	if(fileName==NULL) fileName=m_sFileName;
	if(fileName==NULL) return false;

	u8file *f=u8fopen(fileName,"wb");
	if(f==NULL) return false;

	bool b=SaveWholeFile(f,ret);
	u8fclose(f);

	return b;
}

bool RecordManager::LoadFile(const char* fileName){
	if(fileName==NULL) fileName=m_sFileName;
	if(fileName==NULL) return false;

	CloseFile();

	char sig[16];

	if((m_pFile=u8fopen(fileName,"r+b"))==NULL
		|| u8fread(sig,1,16,m_pFile)!=16
		|| memcmp(sig,"PuzzleBoyRecord\x1A",16)!=0
		)
	{
		if(m_pFile) u8fclose(m_pFile);
		if((m_pFile=u8fopen(fileName,"w+b"))==NULL){
			return false;
		}else{
			u8fwrite("PuzzleBoyRecord\x1A",1,16,m_pFile);
		}
	}

	if(fileName!=m_sFileName){
		if(m_sFileName) SDL_free(m_sFileName);
		m_sFileName=SDL_strdup(fileName);
	}

	//read header
	u8fseek(m_pFile,16,SEEK_SET);
	u8fread(m_nHeader,1,sizeof(m_nHeader),m_pFile);
	SWAP_LE32_ARRAY(m_nHeader);

	//read string
	MySerializer ar;
	int lp=m_nHeader[(1<<HashTableBits)];
	ar.SetFile(m_pFile,false,0);
	while(lp!=0){
		u8fseek(m_pFile,lp,SEEK_SET);
		m_lpLastString=lp;
		int lpNext=ar.GetInt32();
		m_Strings.push_back(ar.GetU16String());
		lp=lpNext;
	}

	//TODO: read unused blocks

	return true;
}

void RecordManager::CloseFile(){
	if(m_pFile){
		u8fclose(m_pFile);
		m_pFile=NULL;
	}

	memset(m_nHeader,0,sizeof(m_nHeader));

	m_Strings.clear();
	m_lpLastString=0;

	/*
	m_nUnusedBlockOffset.RemoveAll();
	m_nUnusedBlockSize.RemoveAll();*/
}

int RecordManager::SerializeAndFindLevel(const PuzzleBoyLevelData& lev,MySerializer& ar,int& idx,unsigned char* bChecksum){
	//some sanity check
	if(m_pFile==NULL) return 0;

	//get level data
	((PuzzleBoyLevelData&)lev).MySerialize(ar);

	//calculate checksum
	ar.CalculateBlake2s(bChecksum,PuzzleBoyLevelData::ChecksumSize);

	//find data
	idx=int(bChecksum[0]) | (int(bChecksum[1]&((1<<(HashTableBits-8))-1))<<8);
	int lp=m_nHeader[idx];
	while(lp>0){
		u8fseek(m_pFile,lp+4,SEEK_SET);
		int lpNext=0;
		u8fread(&lpNext,4,1,m_pFile);
		SWAP_LE32(lpNext);

		unsigned char checksum[PuzzleBoyLevelData::ChecksumSize-1];
		u8fread(checksum,1,sizeof(checksum),m_pFile);

		if(memcmp(bChecksum+1,checksum,sizeof(checksum))==0){
			//found a level with same checksum, assume the level is the same
			//do nothing, exit
#ifdef _DEBUG
			printf("[RecordManager] Debug: Level found\n"); //debug
#endif
			return lp;
		}

		lp=lpNext;
	}

#ifdef _DEBUG
	printf("[RecordManager] Debug: Level not found\n"); //debug
#endif

	return 0;
}

int RecordManager::AddLevel(const PuzzleBoyLevelData& lev){
	//some sanity check
	if(m_pFile==NULL) return 0;

	MySerializer ar,ar2;
	int idx;

	//find level
	unsigned char bChecksum[PuzzleBoyLevelData::ChecksumSize];
	int lp=SerializeAndFindLevel(lev,ar,idx,bChecksum);
	if(lp>0) return lp;

	//not found, add new record
	for(int i=1;i<PuzzleBoyLevelData::ChecksumSize;i++){
		ar2.PutInt8(bChecksum[i]);
	}
	ar2.PutU16String(lev.m_sLevelName); //level name
	ar2->insert(ar2->end(),ar->begin(),ar->end()); //level data

	//get an empty area
	lp=m_nHeader[idx];
	int lpNew=AllocateAndMoveToBlock(ar2->size()+4+4);
	m_nHeader[idx]=lpNew;

	//write data to file
	ar.SetFile(m_pFile,true,16);
	ar.PutInt32(0); //position of first (last) solution
	ar.PutInt32(lp); //position of next level
	ar.FileFlush();
	ar2.WriteFile(m_pFile);

	//write header to file
	u8fseek(m_pFile,16,SEEK_SET);
	WRITE_HEADER(m_nHeader,m_pFile);

	return lpNew;
}

void RecordManager::AddLevelAndRecord(const PuzzleBoyLevelData& lev,int nStep,const u8string& rec,const unsigned short* playerName){
	//some sanity check
	if(m_pFile==NULL) return;

	MySerializer ar2,ar3;

	//add level
	int lpLevel=AddLevel(lev);

	//add player name, no matter it used or not
	int idxName=0;
	if(playerName){
		u16string s=playerName;

		for(idxName=0;idxName<(int)m_Strings.size();idxName++){
			if(s==m_Strings[idxName]) break;
		}
		if(idxName>=(int)m_Strings.size()){
			idxName=m_Strings.size();
			m_Strings.push_back(s);

			ar2.PutInt32(0);
			ar2.PutU16String(s);

			int lpName=AllocateAndMoveToBlock(ar2->size());

			ar2.WriteFile(m_pFile);

			if(m_lpLastString>0){
				u8fseek(m_pFile,m_lpLastString,SEEK_SET);
				int tmp=SDL_SwapLE32(lpName);
				u8fwrite(&tmp,4,1,m_pFile);
			}else{
				m_nHeader[1<<HashTableBits]=lpName;

				//write header to file
				u8fseek(m_pFile,16,SEEK_SET);
				WRITE_HEADER(m_nHeader,m_pFile);
			}

			m_lpLastString=lpName;
		}
	}

	//get record data
	int m=rec.size();
	ar2.Clear();
	ar2.PutVUInt32(idxName); //index of string represents player name.
	ar2.PutVUInt32(nStep); //steps
	ar2.PutVUInt32((m+1)>>1); //solution length (in bytes)

	for(int i=0;i<m;i++){
		int d=0,ch=rec[i];
		switch(ch){
		case 'A':
		case 'a':
			d=10;
			break;
		case 'W':
		case 'w':
			d=11;
			break;
		case 'D':
		case 'd':
			d=12;
			break;
		case 'S':
		case 's':
			d=13;
			break;
		case '(':
		case ',':
		case ')':
			d=14;
			break;
		default:
			if(ch>='0' && ch<='9') d=ch-'0';
		}
		ar2.PutIntN(d,4);
	}
	ar2.PutFlush();

	//check if we should overwrite old record
	ar3.SetFile(m_pFile,false,0);

	int lp=0,lpPrev=lpLevel,lpNext=0;

	u8fseek(m_pFile,lpLevel,SEEK_SET);
	u8fread(&lp,4,1,m_pFile);
	SWAP_LE32(lp);

	while(lp>0){
		u8fseek(m_pFile,lp,SEEK_SET);

		lpNext=0;
		u8fread(&lpNext,4,1,m_pFile); //position of next solution
		SWAP_LE32(lpNext);

		int idxName=ar3.GetVUInt32(); //index of string represents player name
		int st=ar3.GetVUInt32(); //steps
		int sz=ar3.GetVUInt32(); //solution length (in bytes)

		//FIXME: process player name
		if(playerName==NULL || (idxName>=0 && idxName<(int)m_Strings.size() && playerName==m_Strings[idxName])){
			//check if there is a better record already in database
			if(nStep>=st){
#ifdef _DEBUG
				printf("[RecordManager] Debug: Better record already exists\n"); //debug
#endif
				return;
			}

			if(u8ftell(m_pFile)-lp+sz>=(int)ar2->size()+4){
#ifdef _DEBUG
				printf("[RecordManager] Debug: Overwrite existing record, reuse current chunk\n"); //debug
#endif
				//we can reuse this chunk of data
				//do nothing
			}else{
#ifdef _DEBUG
				printf("[RecordManager] Debug: Overwrite existing record, allocate a new chunk\n"); //debug
#endif
				//we should allocate a new one.
				//TODO: mark this chunk unused
				lp=AllocateAndMoveToBlock(ar2->size()+4);

				//changed the linked list
				u8fseek(m_pFile,lpPrev,SEEK_SET);
				int tmp=SDL_SwapLE32(lp);
				u8fwrite(&tmp,4,1,m_pFile);
			}

			break;
		}

		//advance to next record
		lpPrev=lp;
		lp=lpNext;
	}

	//no any record, allocate a new one
	if(lp==0){
#ifdef _DEBUG
		printf("[RecordManager] Debug: Create new record\n"); //debug
#endif
		lp=AllocateAndMoveToBlock(ar2->size()+4);

		//changed the linked list
		u8fseek(m_pFile,lpPrev,SEEK_SET);
		int tmp=SDL_SwapLE32(lp);
		u8fwrite(&tmp,4,1,m_pFile);
	}

	//write record
	u8fseek(m_pFile,lp,SEEK_SET);
	int tmp=SDL_SwapLE32(lpNext);
	u8fwrite(&tmp,4,1,m_pFile);
	ar2.WriteFile(m_pFile);

	//over
}

int RecordManager::FindLevelAndRecord(const PuzzleBoyLevelData& lev,const unsigned short* playerName,u8string* rec,unsigned char* lastChecksum,u16string* returnName){
	//some sanity check
	if(m_pFile==NULL) return -1;

	MySerializer ar,ar3;
	int idx;

	//find level
	unsigned char bChecksum[PuzzleBoyLevelData::ChecksumSize];
	int lp=SerializeAndFindLevel(lev,ar,idx,bChecksum);
	if(lp==0) return -1;

	if(lastChecksum!=NULL && memcmp(bChecksum,lastChecksum,PuzzleBoyLevelData::ChecksumSize)==0) return -2;

	//
	if(returnName) returnName->clear();

	//loop through all records
	ar3.SetFile(m_pFile,false,0);

	int lpPrev=lp,lpNext=0;
	int lpBestRecord=0,nStep=0;

	u8fseek(m_pFile,lp,SEEK_SET);
	lp=0;
	u8fread(&lp,4,1,m_pFile);
	SWAP_LE32(lp);

	while(lp>0){
		u8fseek(m_pFile,lp,SEEK_SET);

		lpNext=0;
		u8fread(&lpNext,4,1,m_pFile); //position of next solution
		SWAP_LE32(lpNext);

		int idxName=ar3.GetVUInt32(); //index of string represents player name
		int st=ar3.GetVUInt32(); //steps

		//check name
		if(playerName==NULL || (idxName>=0 && idxName<(int)m_Strings.size() && m_Strings[idxName]==playerName)){
			//check if this is better
			if(lpBestRecord==0 || st<nStep){
				//record it
				lpBestRecord=lp;
				nStep=st;
				if(returnName) (*returnName)=m_Strings[idxName];
			}else if(st==nStep && returnName){
				const unsigned short s[]={',',' ',0};
				returnName->append(s);
				returnName->append(m_Strings[idxName]);
			}
		}

		//advance to next record
		lpPrev=lp;
		lp=lpNext;
	}

	//check if we found record
	if(lpBestRecord==0) return -1;

	if(rec){
		//read the record
		u8fseek(m_pFile,lpBestRecord+4,SEEK_SET);

		ar3.GetVUInt32(); //index of string represents player name
		nStep=ar3.GetVUInt32(); //steps

		int sz=ar3.GetVUInt32()<<1;
		ConvertRecordDataToString(ar3,sz,*rec);
	}

	//over
	return nStep;
}

void RecordManager::ConvertRecordDataToString(std::vector<unsigned char>& bSolution,u8string& rec){
	rec.clear();
	if(bSolution.empty()) return;

	MySerializer ar3;
	ar3.SetData(&(bSolution[0]),bSolution.size());
	ConvertRecordDataToString(ar3,bSolution.size()*2,rec);
}

void RecordManager::ConvertRecordDataToString(MySerializer& ar3,int sz,u8string& rec){
	int state=0;
	u8string strSwitch;

	rec.clear();
	for(int i=0;i<sz;i++){
		int ch=ar3.GetIntN(4);
		switch(ch){
		case 10:
			if(state==0) rec.push_back('A');
			break;
		case 11:
			if(state==0) rec.push_back('W');
			break;
		case 12:
			if(state==0) rec.push_back('D');
			break;
		case 13:
			if(state==0) rec.push_back('S');
			break;
		case 14:
			if(state==0){
				strSwitch="(";
				state=1;
			}else if(state==1){
				strSwitch.push_back(',');
				state=2;
			}else{
				strSwitch.push_back(')');
				rec.append(strSwitch);
				state=0;
			}
			break;
		default:
			if(ch>=0 && ch<=9 && state>0){
				strSwitch.push_back(ch+'0');
			}
			break;
		}
	}
}

int RecordManager::FindAllRecordsOfLevel(const PuzzleBoyLevelData& lev,std::vector<RecordItem>& ret,unsigned char* lastChecksum){
	//some sanity check
	if(m_pFile==NULL){
		ret.clear();
		return -1;
	}

	MySerializer ar,ar3;
	int idx;

	//find level
	unsigned char bChecksum[PuzzleBoyLevelData::ChecksumSize];
	int lp=SerializeAndFindLevel(lev,ar,idx,bChecksum);
	if(lp==0){
		ret.clear();
		return -1;
	}

	if(lastChecksum!=NULL && memcmp(bChecksum,lastChecksum,PuzzleBoyLevelData::ChecksumSize)==0) return -2;

	ret.clear();

	//loop through all records
	ar3.SetFile(m_pFile,false,0);

	int lpPrev=lp,lpNext=0;
	int nBestRecord=0;

	u8fseek(m_pFile,lp,SEEK_SET);
	lp=0;
	u8fread(&lp,4,1,m_pFile);
	SWAP_LE32(lp);

	while(lp>0){
		u8fseek(m_pFile,lp,SEEK_SET);

		lpNext=0;
		u8fread(&lpNext,4,1,m_pFile); //position of next solution
		SWAP_LE32(lpNext);

		RecordItem rec;
		int idxName=ar3.GetVUInt32(); //index of string represents player name
		rec.nStep=ar3.GetVUInt32(); //steps
		int sz=ar3.GetVUInt32(); //size of solution data
		if(sz>0){
			rec.bSolution.resize(sz);
			u8fread(&(rec.bSolution[0]),1,sz,m_pFile); //load solution data
		}

		//check best record
		if(nBestRecord==0 || nBestRecord>rec.nStep){
			nBestRecord=rec.nStep;
		}

		//check name
		if(idxName>=0 && idxName<(int)m_Strings.size()){
			rec.sPlayerName=m_Strings[idxName];
		}

		//add to result
		ret.push_back(rec);

		//advance to next record
		lpPrev=lp;
		lp=lpNext;
	}

	return nBestRecord;
}

int RecordManager::GetLevelCount(){
	//some sanity check
	if(m_pFile==NULL) return 0;

	int count=0;

	for(int idx=0;idx<(1<<HashTableBits);idx++){
		int lp=m_nHeader[idx];
		while(lp>0){
			u8fseek(m_pFile,lp+4,SEEK_SET);
			lp=0;
			u8fread(&lp,4,1,m_pFile);
			SWAP_LE32(lp);

			count++;
		}
	}

	return count;
}

int RecordManager::AllocateAndMoveToBlock(int nSize){
	//some sanity check
	if(m_pFile==NULL) return 0;

	//TODO: use unused blocks

	u8fseek(m_pFile,0,SEEK_END);
	int lp=u8ftell(m_pFile);

	if(lp<16+sizeof(m_nHeader)){
		//rewrite the file (???)
		u8fseek(m_pFile,0,SEEK_SET);
		u8fwrite("PuzzleBoyRecord\x1A",1,16,m_pFile);
		WRITE_HEADER(m_nHeader,m_pFile);
		lp=u8ftell(m_pFile);
	}

	return lp;
}
