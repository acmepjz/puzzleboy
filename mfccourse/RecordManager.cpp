#include "stdafx.h"
#include "RecordManager.h"
#include "MySerializer.h"
#include "PushableBlock.h"
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
		free(m_sFileName);
		m_sFileName=NULL;
	}
}

bool RecordManager::LoadWholeFile(FILE* f,RecordFile& ret){
	char sig[16];
	int nHeader[(1UL<<HashTableBits)+8];

	//read header
	fseek(f,0,SEEK_SET);
	if(fread(sig,1,16,f)!=16 || memcmp(sig,"PuzzleBoyRecord\x1A",16)!=0){
		return false;
	}

	memset(nHeader,0,sizeof(nHeader));
	fread(nHeader,1,sizeof(nHeader),f);

	//load player name
	MySerializer ar;

	int lp=nHeader[(1<<HashTableBits)];
	ar.SetFile(f,false,0);

	CArray<CString> sStrings;

	while(lp!=0){
		fseek(f,lp,SEEK_SET);
		int lpNext=ar.GetInt32();
		sStrings.Add(ar.GetString());
		lp=lpNext;
	}

	//read all level
	ret.objLevels.clear();

	for(int idx=0;idx<(1<<HashTableBits);idx++){
		lp=nHeader[idx];
		while(lp>0){
			fseek(f,lp,SEEK_SET);

			int lpRecord=0;
			fread(&lpRecord,4,1,f); //position of first (last) solution
			lp=0;
			fread(&lp,4,1,f); //position of next level

			RecordLevelItem lev;
			lev.objChecksum[0]=idx>>0;
			fread(&(lev.objChecksum[1]),1,CLevel::ChecksumSize-1,f); //BLAKE2s-160 checksum

			lev.sName=ar.GetString(); //level name

			//parse level data
			int lpStart=ftell(f);
			lev.nWidth=ar.GetVUInt32();
			lev.nHeight=ar.GetVUInt32();
			lev.nBestRecord=0;
			lev.nUserData=0;

			int m=(lev.nWidth*lev.nHeight+1)>>1;
			fseek(f,m,SEEK_CUR);

			m=ar.GetVUInt32();
			for(int i=0;i<m;i++){
				int nType=ar.GetVUInt32();
				ar.GetVUInt32();
				ar.GetVUInt32();

				if(nType!=ROTATE_BLOCK){
					int w=ar.GetVUInt32();
					int h=ar.GetVUInt32();
					fseek(f,(w*h+7)>>3,SEEK_CUR);
				}else{
					fseek(f,4,SEEK_CUR);
				}
			}

			//calculate level data size and read
			m=ftell(f)-lpStart;
			if(m>0){
				lev.bData.resize(m);
				fseek(f,lpStart,SEEK_SET);
				fread(&(lev.bData[0]),1,m,f);
			}

			//read all records
			while(lpRecord>0){
				fseek(f,lpRecord,SEEK_SET);

				lpRecord=0;
				fread(&lpRecord,4,1,f); //position of next solution

				int idxName=ar.GetVUInt32(); //index of string represents player name
				int st=ar.GetVUInt32(); //steps
				int sz=ar.GetVUInt32(); //solution length (in bytes)

				RecordItem rec;
				if(idxName>=0 && idxName<sStrings.GetSize()){
					rec.sPlayerName=sStrings[idxName];
				}
				rec.nStep=st;

				if(sz>0){
					rec.bSolution.resize(sz);
					fread(&(rec.bSolution[0]),1,sz,f);
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

bool RecordManager::SaveWholeFile(FILE* f,RecordFile& ret){
	MySerializer ar,ar2;
	int nHeader[(1UL<<HashTableBits)+8];
	int nPrevious[1UL<<HashTableBits];

	CArray<CString> sStrings;
	std::map<CString,int>  mapStrings;

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
			memcpy(&(ar[nPrevious[idx]+4-(16+sizeof(nHeader))]),&lp,4);
		}else{
			nHeader[idx]=lp;
		}

		//set current level
		nPrevious[idx]=lp;

		//create level data
		ar2.Clear();
		ar2.PutInt32(0); //position of first (last) solution
		ar2.PutInt32(0); //position of next level
		for(int k=1;k<CLevel::ChecksumSize;k++){
			ar2.PutInt8(lev.objChecksum[k]); //BLAKE2s-160 checksum
		}
		ar2.PutString(lev.sName); //level name

		if(lev.bData.size()>0){
			int m=ar2->GetSize();
			ar2->SetSize(m+lev.bData.size());
			memcpy(&(ar2[m]),&(lev.bData[0]),lev.bData.size()); //level data
		}

		//save level
		ar->Append(ar2.GetData());
		lp+=ar2->GetSize();

		//save all records
		for(unsigned int j=0;j<lev.objRecord.size();j++){
			//link previous record to this record
			memcpy(&(ar[nPreviousRecord-(16+sizeof(nHeader))]),&lp,4);

			//set current record
			nPreviousRecord=lp;

			//create record data
			ar2.Clear();
			ar2.PutInt32(0); //position of next solution

			int idxName=0;
			std::map<CString,int>::const_iterator it=mapStrings.find(lev.objRecord[j].sPlayerName);
			if(it==mapStrings.end()){
				//add new name
				idxName=sStrings.GetSize();
				sStrings.Add(lev.objRecord[j].sPlayerName);
				mapStrings[lev.objRecord[j].sPlayerName]=idxName;
			}else{
				idxName=it->second;
			}
			ar2.PutVUInt32(idxName); //index of string represents player name

			ar2.PutVUInt32(lev.objRecord[j].nStep); //steps

			int n=lev.objRecord[j].bSolution.size();
			ar2.PutVUInt32(n); //solution length (in bytes)
			if(n>0){
				int m=ar2->GetSize();
				ar2->SetSize(m+n);
				memcpy(&(ar2[m]),&(lev.objRecord[j].bSolution[0]),n); //solution data
			}

			//save record
			ar->Append(ar2.GetData());
			lp+=ar2->GetSize();
		}
	}

	//save player names
	nHeader[1UL<<HashTableBits]=lp;
	for(int i=0;i<sStrings.GetSize();i++){
		ar2.Clear();
		ar2.PutInt32(0); //position of next string
		ar2.PutString(sStrings[i]); //string

		int lpNext=lp+ar2->GetSize();

		if(i<sStrings.GetSize()-1){
			memcpy(&(ar2[0]),&lpNext,4); //position of next string
		}

		ar->Append(ar2.GetData());
		lp=lpNext;
	}

	//save header
	fwrite("PuzzleBoyRecord\x1A",1,16,f);
	fwrite(nHeader,1,sizeof(nHeader),f);

	//save data
	ar.WriteFile(f);

	return true;
}

bool RecordManager::SaveWholeFile(RecordFile& ret,const char* fileName){
	if(fileName==NULL) fileName=m_sFileName;
	if(fileName==NULL) return false;

	FILE *f=fopen(fileName,"wb");
	if(f==NULL) return false;

	bool b=SaveWholeFile(f,ret);
	fclose(f);

	return b;
}

bool RecordManager::LoadFile(const char* fileName){
	if(fileName==NULL) fileName=m_sFileName;
	if(fileName==NULL) return false;

	CloseFile();

	char sig[16];

	if((m_pFile=fopen(fileName,"r+b"))==NULL
		|| fread(sig,1,16,m_pFile)!=16
		|| memcmp(sig,"PuzzleBoyRecord\x1A",16)!=0
		)
	{
		if(m_pFile) fclose(m_pFile);
		if((m_pFile=fopen(fileName,"w+b"))==NULL){
			return false;
		}else{
			fwrite("PuzzleBoyRecord\x1A",1,16,m_pFile);
		}
	}

	if(fileName!=m_sFileName){
		if(m_sFileName) free(m_sFileName);
		m_sFileName=_strdup(fileName);
	}

	//read header
	fseek(m_pFile,16,SEEK_SET);
	fread(m_nHeader,1,sizeof(m_nHeader),m_pFile);

	//read string
	MySerializer ar;
	int lp=m_nHeader[(1<<HashTableBits)];
	ar.SetFile(m_pFile,false,0);
	while(lp!=0){
		fseek(m_pFile,lp,SEEK_SET);
		m_lpLastString=lp;
		int lpNext=ar.GetInt32();
		m_Strings.Add(ar.GetString());
		lp=lpNext;
	}

	//TODO: read unused blocks

	return true;
}

void RecordManager::CloseFile(){
	if(m_pFile){
		fclose(m_pFile);
		m_pFile=NULL;
	}

	memset(m_nHeader,0,sizeof(m_nHeader));

	m_Strings.RemoveAll();
	m_lpLastString=0;

	/*
	m_nUnusedBlockOffset.RemoveAll();
	m_nUnusedBlockSize.RemoveAll();*/
}

int RecordManager::SerializeAndFindLevel(CLevel& lev,MySerializer& ar,int& idx,unsigned char* bChecksum){
	//get level data
	lev.MySerialize(ar);

	//calculate checksum
	ar.CalculateBlake2s(bChecksum,CLevel::ChecksumSize);

	//find data
	idx=int(bChecksum[0]) | (int(bChecksum[1]&((1<<(HashTableBits-8))-1))<<8);
	int lp=m_nHeader[idx];
	while(lp>0){
		fseek(m_pFile,lp+4,SEEK_SET);
		int lpNext=0;
		fread(&lpNext,4,1,m_pFile);

		unsigned char checksum[CLevel::ChecksumSize-1];
		fread(checksum,1,sizeof(checksum),m_pFile);

		if(memcmp(bChecksum+1,checksum,sizeof(checksum))==0){
			//found a level with same checksum, assume the level is the same
			//do nothing, exit
#ifdef _DEBUG
			::OutputDebugString(_T("Level found\n")); //debug
#endif
			return lp;
		}

		lp=lpNext;
	}

#ifdef _DEBUG
	::OutputDebugString(_T("Level not found\n")); //debug
#endif

	return 0;
}

int RecordManager::AddLevel(CLevel& lev){
	MySerializer ar,ar2;
	int idx;

	//find level
	unsigned char bChecksum[CLevel::ChecksumSize];
	int lp=SerializeAndFindLevel(lev,ar,idx,bChecksum);
	if(lp>0) return lp;

	//not found, add new record
	for(int i=1;i<CLevel::ChecksumSize;i++){
		ar2.PutInt8(bChecksum[i]);
	}
	ar2.PutString(lev.m_sLevelName); //level name
	ar2->Append(ar.GetData()); //level data

	//get an empty area
	lp=m_nHeader[idx];
	int lpNew=AllocateAndMoveToBlock(ar2->GetSize()+4+4);
	m_nHeader[idx]=lpNew;

	//write data to file
	ar.SetFile(m_pFile,true,16);
	ar.PutInt32(0); //position of first (last) solution
	ar.PutInt32(lp); //position of next level
	ar.FileFlush();
	ar2.WriteFile(m_pFile);

	//write header to file
	fseek(m_pFile,16,SEEK_SET);
	fwrite(m_nHeader,1,sizeof(m_nHeader),m_pFile);

	return lpNew;
}

void RecordManager::AddLevelAndRecord(CLevel& lev,int nStep,const CString& rec,const wchar_t* playerName){
	MySerializer ar2,ar3;

	//add level
	int lpLevel=AddLevel(lev);

	//add player name, no matter it used or not
	int idxName=0;
	if(playerName){
		CString s=playerName;

		for(idxName=0;idxName<m_Strings.GetSize();idxName++){
			if(s==m_Strings[idxName]) break;
		}
		if(idxName>=m_Strings.GetSize()){
			idxName=m_Strings.GetSize();
			m_Strings.Add(s);

			ar2.PutInt32(0);
			ar2.PutString(s);

			int lpName=AllocateAndMoveToBlock(ar2->GetSize());

			ar2.WriteFile(m_pFile);

			if(m_lpLastString>0){
				fseek(m_pFile,m_lpLastString,SEEK_SET);
				fwrite(&lpName,4,1,m_pFile);
			}else{
				m_nHeader[1<<HashTableBits]=lpName;

				//write header to file
				fseek(m_pFile,16,SEEK_SET);
				fwrite(m_nHeader,1,sizeof(m_nHeader),m_pFile);
			}

			m_lpLastString=lpName;
		}
	}

	//get record data
	int m=rec.GetLength();
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

	fseek(m_pFile,lpLevel,SEEK_SET);
	fread(&lp,4,1,m_pFile);

	while(lp>0){
		fseek(m_pFile,lp,SEEK_SET);

		lpNext=0;
		fread(&lpNext,4,1,m_pFile); //position of next solution

		int idxName=ar3.GetVUInt32(); //index of string represents player name
		int st=ar3.GetVUInt32(); //steps
		int sz=ar3.GetVUInt32(); //solution length (in bytes)

		//FIXME: process player name
		if(playerName==NULL || (idxName>=0 && idxName<m_Strings.GetSize() && CString(playerName)==m_Strings[idxName])){
			//check if there is a better record already in database
			if(nStep>=st){
#ifdef _DEBUG
				::OutputDebugString(_T("Better record already exists\n")); //debug
#endif
				return;
			}

			if(ftell(m_pFile)-lp+sz>=ar2->GetSize()+4){
#ifdef _DEBUG
				::OutputDebugString(_T("Overwrite existing record, reuse current chunk\n")); //debug
#endif
				//we can reuse this chunk of data
				//do nothing
			}else{
#ifdef _DEBUG
				::OutputDebugString(_T("Overwrite existing record, allocate a new chunk\n")); //debug
#endif
				//we should allocate a new one.
				//TODO: mark this chunk unused
				lp=AllocateAndMoveToBlock(ar2->GetSize()+4);

				//changed the linked list
				fseek(m_pFile,lpPrev,SEEK_SET);
				fwrite(&lp,4,1,m_pFile);
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
		::OutputDebugString(_T("Create new record\n")); //debug
#endif
		lp=AllocateAndMoveToBlock(ar2->GetSize()+4);

		//changed the linked list
		fseek(m_pFile,lpPrev,SEEK_SET);
		fwrite(&lp,4,1,m_pFile);
	}

	//write record
	fseek(m_pFile,lp,SEEK_SET);
	fwrite(&lpNext,4,1,m_pFile);
	ar2.WriteFile(m_pFile);

	//over
}

int RecordManager::FindLevelAndRecord(CLevel& lev,const wchar_t* playerName,CString* rec,unsigned char* lastChecksum,CString* returnName){
	MySerializer ar,ar3;
	int idx;

	//find level
	unsigned char bChecksum[CLevel::ChecksumSize];
	int lp=SerializeAndFindLevel(lev,ar,idx,bChecksum);
	if(lp==0) return -1;

	if(lastChecksum!=NULL && memcmp(bChecksum,lastChecksum,CLevel::ChecksumSize)==0) return -2;

	//
	if(returnName) returnName->Empty();

	//loop through all records
	ar3.SetFile(m_pFile,false,0);

	int lpPrev=lp,lpNext=0;
	int lpBestRecord=0,nStep=0;

	fseek(m_pFile,lp,SEEK_SET);
	lp=0;
	fread(&lp,4,1,m_pFile);

	while(lp>0){
		fseek(m_pFile,lp,SEEK_SET);

		lpNext=0;
		fread(&lpNext,4,1,m_pFile); //position of next solution

		int idxName=ar3.GetVUInt32(); //index of string represents player name
		int st=ar3.GetVUInt32(); //steps

		//check name
		if(playerName==NULL || (idxName>=0 && idxName<m_Strings.GetSize() && m_Strings[idxName]==playerName)){
			//check if this is better
			if(lpBestRecord==0 || st<nStep){
				//record it
				lpBestRecord=lp;
				nStep=st;
				if(returnName) (*returnName)=m_Strings[idxName];
			}else if(st==nStep && returnName){
				returnName->AppendFormat(_T(", %s"),(LPCTSTR)m_Strings[idxName]);
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
		fseek(m_pFile,lpBestRecord+4,SEEK_SET);

		ar3.GetVUInt32(); //index of string represents player name
		nStep=ar3.GetVUInt32(); //steps

		int sz=ar3.GetVUInt32()<<1;
		ConvertRecordDataToString(ar3,sz,*rec);
	}

	//over
	return nStep;
}

void RecordManager::ConvertRecordDataToString(std::vector<unsigned char>& bSolution,CString& rec){
	rec.Empty();
	if(bSolution.empty()) return;

	MySerializer ar3;
	ar3.SetData(&(bSolution[0]),bSolution.size());
	ConvertRecordDataToString(ar3,bSolution.size()*2,rec);
}

void RecordManager::ConvertRecordDataToString(MySerializer& ar3,int sz,CString& rec){
	int state=0;
	CString strSwitch;

	rec.Empty();
	for(int i=0;i<sz;i++){
		int ch=ar3.GetIntN(4);
		switch(ch){
		case 10:
			if(state==0) rec.AppendChar('A');
			break;
		case 11:
			if(state==0) rec.AppendChar('W');
			break;
		case 12:
			if(state==0) rec.AppendChar('D');
			break;
		case 13:
			if(state==0) rec.AppendChar('S');
			break;
		case 14:
			if(state==0){
				strSwitch=_T("(");
				state=1;
			}else if(state==1){
				strSwitch.AppendChar(',');
				state=2;
			}else{
				strSwitch.AppendChar(')');
				rec.Append(strSwitch);
				state=0;
			}
			break;
		default:
			if(ch>=0 && ch<=9 && state>0){
				strSwitch.AppendChar(ch+'0');
			}
			break;
		}
	}
}

int RecordManager::FindAllRecordsOfLevel(CLevel& lev,std::vector<RecordItem>& ret,unsigned char* lastChecksum){
	MySerializer ar,ar3;
	int idx;

	//find level
	unsigned char bChecksum[CLevel::ChecksumSize];
	int lp=SerializeAndFindLevel(lev,ar,idx,bChecksum);
	if(lp==0){
		ret.clear();
		return -1;
	}

	if(lastChecksum!=NULL && memcmp(bChecksum,lastChecksum,CLevel::ChecksumSize)==0) return -2;

	ret.clear();

	//loop through all records
	ar3.SetFile(m_pFile,false,0);

	int lpPrev=lp,lpNext=0;
	int nBestRecord=0;

	fseek(m_pFile,lp,SEEK_SET);
	lp=0;
	fread(&lp,4,1,m_pFile);

	while(lp>0){
		fseek(m_pFile,lp,SEEK_SET);

		lpNext=0;
		fread(&lpNext,4,1,m_pFile); //position of next solution

		RecordItem rec;
		int idxName=ar3.GetVUInt32(); //index of string represents player name
		rec.nStep=ar3.GetVUInt32(); //steps
		int sz=ar3.GetVUInt32(); //size of solution data
		if(sz>0){
			rec.bSolution.resize(sz);
			fread(&(rec.bSolution[0]),1,sz,m_pFile); //load solution data
		}

		//check best record
		if(nBestRecord==0 || nBestRecord>rec.nStep){
			nBestRecord=rec.nStep;
		}

		//check name
		if(idxName>=0 && idxName<m_Strings.GetSize()){
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
	int count=0;

	for(int idx=0;idx<(1<<HashTableBits);idx++){
		int lp=m_nHeader[idx];
		while(lp>0){
			fseek(m_pFile,lp+4,SEEK_SET);
			lp=0;
			fread(&lp,4,1,m_pFile);

			count++;
		}
	}

	return count;
}

int RecordManager::AllocateAndMoveToBlock(int nSize){
	//TODO: use unused blocks

	fseek(m_pFile,0,SEEK_END);
	int lp=ftell(m_pFile);

	if(lp<16+sizeof(m_nHeader)){
		//rewrite the file (???)
		fseek(m_pFile,0,SEEK_SET);
		fwrite("PuzzleBoyRecord\x1A",1,16,m_pFile);
		fwrite(m_nHeader,1,sizeof(m_nHeader),m_pFile);
		lp=ftell(m_pFile);
	}

	return lp;
}
