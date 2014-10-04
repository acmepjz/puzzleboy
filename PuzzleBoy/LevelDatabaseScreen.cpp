#include "LevelDatabaseScreen.h"
#include "SimpleText.h"
#include "PuzzleBoyApp.h"
#include "main.h"
#include "PuzzleBoyLevelFile.h"
#include "FileSystem.h"
#include "SimpleMiscScreen.h"
#include "SimpleMessageBox.h"
#include "MyFormat.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <SDL.h>

#define NEW_LEVEL 0x1
#define NEW_RECORD 0x2

class MergeDatabaseScreen:public SimpleListScreen{
public:
	MergeDatabaseScreen(RecordFile& tFile):m_tFile(tFile){
	}
	void OnDirty() override{
		ResetList();

		AddItem(_("Choose level database to be merged."),
			true,0,float(screenWidth-4),DrawTextFlags::AutoSize);
		AddItem(_("Note: PuzzleBoyRecord.dat is our main database."),
			true,0,float(screenWidth-4),DrawTextFlags::AutoSize);
		AddItem(str(MyFormat(_("These files can be found at %s."))<<externalStoragePath),
			true,0,float(screenWidth-4),DrawTextFlags::AutoSize);

		AddEmptyItem();

		for(int i=0,m=m_files.size();i<m;i++){
			AddItem(m_files[i]);
		}
	}
	int OnClick(int index) override{
		index-=4;

		if(index>=0 && index<(int)m_files.size()){
			//close main database
			theApp->m_objRecordMgr.CloseFile();

			//open file
			u8string fileName=externalStoragePath+"/"+m_files[index];
			u8file *f=u8fopen(fileName.c_str(),"rb");

			RecordFile lvs;
			int nNewLevel=0,nNewRecord=0;

			bool b=(f!=NULL && RecordManager::LoadWholeFile(f,lvs));

			if(b){
				//build a map for existing levels
				std::map<RecordLevelChecksum,unsigned int> mp;

				//add all existing levels to the map
				for(unsigned int i=0;i<m_tFile.objLevels.size();i++){
					if(m_tFile.objLevels[i].bData.empty()) continue;
					mp[m_tFile.objLevels[i].objChecksum]=i;
				}

				//check newly added levels
				for(unsigned int i=0;i<lvs.objLevels.size();i++){
					RecordLevelItem& lev=lvs.objLevels[i];

					if(lev.bData.empty()) continue;

					std::map<RecordLevelChecksum,unsigned int>::const_iterator
						it=mp.find(lev.objChecksum);

					if(it==mp.end()){
						//just add this level
						lev.nUserData=NEW_LEVEL;
						m_tFile.objLevels.push_back(lev);
						mp[lev.objChecksum]=m_tFile.objLevels.size()-1;

						nNewLevel++;
					}else{
						//check records
						std::map<u16string,unsigned int> mapPlayerNames;

						//first get existing records
						RecordLevelItem& lev0=m_tFile.objLevels[it->second];
						for(unsigned int j=0,m=lev0.objRecord.size();j<m;j++){
							mapPlayerNames[lev0.objRecord[j].sPlayerName]=j;
						}

						//check new records
						for(unsigned int j=0,m=lev.objRecord.size();j<m;j++){
							std::map<u16string,unsigned int>::const_iterator
								it=mapPlayerNames.find(lev.objRecord[j].sPlayerName);

							if(it==mapPlayerNames.end()){
								//no record, just add it
								mapPlayerNames[lev.objRecord[j].sPlayerName]=lev0.objRecord.size();
								lev0.nUserData|=NEW_RECORD;
								lev0.objRecord.push_back(lev.objRecord[j]);
								nNewRecord++;
							}else if(lev.objRecord[j].nStep<lev0.objRecord[it->second].nStep){
								//replace record
								lev0.nUserData|=NEW_RECORD;
								lev0.objRecord[it->second]=lev.objRecord[j];
								nNewRecord++;
							}
						}

						//update best record
						lev0.nBestRecord=0;
						for(unsigned int j=0,m=lev0.objRecord.size();j<m;j++){
							if(lev0.nBestRecord==0 || lev0.nBestRecord>lev0.objRecord[j].nStep){
								lev0.nBestRecord=lev0.objRecord[j].nStep;
							}
						}
					}
				}
			}

			if(f) u8fclose(f);

			if(b){
				theApp->ShowToolTip(str(MyFormat(_("Successfully added %d levels and %d best records"))<<nNewLevel<<nNewRecord));
				return 1;
			}else{
				theApp->ShowToolTip(str(MyFormat(_("Failed to load file %s"))<<fileName));
			}
		}
		return -1;
	}
	int DoModal() override{
		m_bDirtyOnResize=true;

		m_files=enumAllFiles(externalStoragePath,"dat");

		m_titleBar.m_sTitle=_("Merge Level Database");
		return SimpleListScreen::DoModal();
	}
private:
	std::vector<u8string> m_files;
	RecordFile& m_tFile;
};

class LevelInformationScreen:public SimpleListScreen{
public:
	LevelInformationScreen(RecordFile& tFile,int index):m_tFile(tFile),m_nIndex(index){
	}
	void OnDirty() override{
		ResetList();

		RecordLevelItem& lev=m_tFile.objLevels[m_nIndex];

		//name
		AddItem(str(MyFormat(_("Level Name"))(": %s")<<toUTF8(lev.sName)),
			true,0,float(screenWidth),DrawTextFlags::AutoSize);

		//size
		AddItem(str(MyFormat(_("Level Size"))(": %dx%d")<<lev.nWidth<<lev.nHeight));

		//hash
		{
			u8string s=_("Checksum")+": ";
			for(int i=0;i<PuzzleBoyLevelData::ChecksumSize;i+=4){
				unsigned int c=((unsigned int)(lev.objChecksum[i])<<24)
					| ((unsigned int)(lev.objChecksum[i+1])<<16)
					| ((unsigned int)(lev.objChecksum[i+2])<<8)
					| (unsigned int)(lev.objChecksum[i+3]);
				char sss[12];
				sprintf(sss,"%08X",c);
				s.append(sss);
			}
			AddItem(s,true,0,float(screenWidth),DrawTextFlags::AutoSize);
		}

		//best record
		{
			u8string s=_("Best Record")+": ";
			if(lev.objRecord.empty()){
				s+=_("Doesn't Exist");
			}else{
				int st=lev.nBestRecord;

				char sss[16];
				sprintf(sss,"%d (",st);
				s.append(sss);

				int j=0;
				for(int i=0,m=lev.objRecord.size();i<m;i++){
					if(lev.objRecord[i].nStep==st){
						if(j>0) s.append(", ");
						s.append(toUTF8(lev.objRecord[i].sPlayerName));
						j++;
					}
				}

				s.push_back(')');
			}
			AddItem(s,true,0,float(screenWidth),DrawTextFlags::AutoSize);
		}

		AddEmptyItem();

		//menu
		AddItem(_("Show Level")); //index=5
		AddItem(_("Remove Level")); //index=6

		AddEmptyItem();

		//record list
		AddItem(_("List of Records")+':');

		for(int i=0,m=lev.objRecord.size();i<m;i++){
			char s[16];
			sprintf(s,": %d",lev.objRecord[i].nStep);
			AddItem(toUTF8(lev.objRecord[i].sPlayerName)+s); //index start from 9
		}
	}
	int OnClick(int index) override{
		m_nListIndex=-1;

		m_titleBar.m_RightButtons.clear();

		if(index==5){
			//show level
			theApp->m_pDocument->CreateNew();
			theApp->m_pDocument->m_objLevels[0]->m_sLevelName=m_tFile.objLevels[m_nIndex].sName;

			MySerializer ar;
			ar.SetData(m_tFile.objLevels[m_nIndex].bData);
			theApp->m_pDocument->m_objLevels[0]->MySerialize(ar);

			return 1;
		}else if(index==6){
			//delete level (msgbox userdata=-1)
			delete m_msgBox;
			m_msgBox=new SimpleMessageBox();

			m_msgBox->m_nUserData=-1;
			m_msgBox->m_prompt=_("Do you want to remove this level?");
			m_msgBox->m_buttons.push_back(_("Yes"));
			m_msgBox->m_buttons.push_back(_("No"));
			m_msgBox->Create();
			m_msgBox->m_nDefaultValue=0;
			m_msgBox->m_nCancelValue=1;
		}else if(index>=9 && index<9+(int)m_tFile.objLevels[m_nIndex].objRecord.size()){
			m_titleBar.m_RightButtons.push_back(SCREEN_KEYBOARD_COPY);
			m_titleBar.m_RightButtons.push_back(SCREEN_KEYBOARD_DELETE);
			m_nListIndex=index;
		}

		RecreateTitleBar();

		return -1;
	}
	int OnTitleBarButtonClick(int index) override{
		switch(index){
		case SCREEN_KEYBOARD_LEFT:
			return m_nReturnValue;
			break;
		case SCREEN_KEYBOARD_COPY:
			index=m_nListIndex-9;
			if(index>=0 && index<(int)m_tFile.objLevels[m_nIndex].objRecord.size()){
				u8string s;
				RecordManager::ConvertRecordDataToString(m_tFile.objLevels[m_nIndex].objRecord[index].bSolution,s);
				SDL_SetClipboardText(s.c_str());
				theApp->ShowToolTip(_("Copied to clipboard"));

				//ad-hoc fix
				m_nIdleTime=0;
			}
			break;
		case SCREEN_KEYBOARD_DELETE:
			//delete record (msgbox userdata=record index)
			index=m_nListIndex-9;
			if(index>=0 && index<(int)m_tFile.objLevels[m_nIndex].objRecord.size()){
				delete m_msgBox;
				m_msgBox=new SimpleMessageBox();

				m_msgBox->m_nUserData=index;
				m_msgBox->m_prompt=str(MyFormat(_("Do you want to remove level record for '%s'?"))
					<<toUTF8(m_tFile.objLevels[m_nIndex].objRecord[index].sPlayerName));
				m_msgBox->m_buttons.push_back(_("Yes"));
				m_msgBox->m_buttons.push_back(_("No"));
				m_msgBox->Create();
				m_msgBox->m_nDefaultValue=0;
				m_msgBox->m_nCancelValue=1;

				//ad-hoc fix
				m_nIdleTime=0;
			}
			break;
		default:
			break;
		}

		return -1;
	}
	int DoModal() override{
		m_bDirtyOnResize=true;
		m_titleBar.m_sTitle=_("Level Information");
		return SimpleListScreen::DoModal();
	}
	int OnMsgBoxClick(int index) override{
		if(m_msgBox){
			int userData=(int)(m_msgBox->m_nUserData);

			delete m_msgBox;
			m_msgBox=NULL;

			if(index==0){
				if(userData<0){
					//remove level (logically)
					m_tFile.objLevels[m_nIndex].bData.clear();
					return 2;
				}else if(userData<(int)m_tFile.objLevels[m_nIndex].objRecord.size()){
					//remove level record
					m_tFile.objLevels[m_nIndex].objRecord.erase(
						m_tFile.objLevels[m_nIndex].objRecord.begin()+userData
						);

					//update best record
					int st=0;
					for(int i=0,m=m_tFile.objLevels[m_nIndex].objRecord.size();i<m;i++){
						if(st==0 || st>m_tFile.objLevels[m_nIndex].objRecord[i].nStep){
							st=m_tFile.objLevels[m_nIndex].objRecord[i].nStep;
						}
					}
					m_tFile.objLevels[m_nIndex].nBestRecord=st;

					m_nReturnValue=2;

					m_nListIndex=-1;
					RecreateTitleBar();

					m_bDirty=true;
				}
			}
		}

		return -1;
	}
private:
	RecordFile& m_tFile;
	const int m_nIndex;
};

void LevelDatabaseScreen::OnDirty(){
	if(m_searchFilter.empty()){
		m_titleBar.m_sTitle=_("Level Database");
	}else{
		m_titleBar.m_sTitle=_("Search Result");
	}

	RecreateTitleBar();

	ResetList();

	const int w1=theApp->m_nMenuTextSize*5; //160
	const int w2=120+theApp->m_nMenuTextSize*5; //288

	for(int index=0,count=m_searchResult.size();index<count;index++){
		int i=m_searchResult[index];

		RecordLevelItem& lev=m_tFile.objLevels[i];

		assert(!lev.bData.empty()); //empty means deleted logically

		char s[32];

		//hash
		unsigned int c1=((unsigned int)(lev.objChecksum[0])<<8)
			| (unsigned int)(lev.objChecksum[1]),
			c2=((unsigned int)(lev.objChecksum[PuzzleBoyLevelData::ChecksumSize-2])<<8)
			| (unsigned int)(lev.objChecksum[PuzzleBoyLevelData::ChecksumSize-1]);
		sprintf(s,"%04X..%04X",c1,c2);
		AddItem(s,true,0,108,DrawTextFlags::AutoSize);

		//name
		AddItem(toUTF8(lev.sName),false,
			112,float(screenWidth-w2),DrawTextFlags::AutoSize);

		//size
		sprintf(s,"%dx%d",lev.nWidth,lev.nHeight);
		AddItem(s,false,float(screenWidth-w1));

		//record
		if(lev.objRecord.empty()) strcpy(s,"---");
		else sprintf(s,"%d",lev.nBestRecord);
		AddItem(s,false,float(screenWidth),0,DrawTextFlags::Right);

		//update selection
		if(lev.objChecksum==m_tSelected) m_nListIndex=ListCount()-1;
	}

	EnsureVisible();
}

int LevelDatabaseScreen::OnClick(int index){
	m_nListIndex=index;

	if(index>=0 && index<(int)m_searchResult.size()) index=m_searchResult[index];
	else index=-1;

	if(index<0 || index>=(int)m_tFile.objLevels.size()) return -1;

	m_tSelected=m_tFile.objLevels[index].objChecksum;

	//1=show level, 2=level database updated
	int ret=LevelInformationScreen(m_tFile,index).DoModal();

	switch(ret){
	case 1:
		return 1;
		break;
	case 2:
		FilterDirty();
		break;
	}

	return -1;
}

int LevelDatabaseScreen::OnTitleBarButtonClick(int index){
	switch(index){
	case SCREEN_KEYBOARD_LEFT:
		if(m_searchFilter.empty()) return 0;
		//clear search filter and display again
		m_searchFilter.clear();
		FilterDirty();
		break;
	case SCREEN_KEYBOARD_SEARCH:
		if(SimpleInputScreen(
			_("Search Level"),
			_("Please input the keyword"),
			m_searchFilter))
		{
			//trim the string
			u8string::size_type lps=m_searchFilter.find_first_not_of(" \t\r\n");
			if(lps==u8string::npos){
				m_searchFilter.clear();
			}else{
				m_searchFilter=m_searchFilter.substr(lps);
				lps=m_searchFilter.find_last_not_of(" \t\r\n");
				m_searchFilter=m_searchFilter.substr(0,lps+1);
			}

			FilterDirty();
		}
		break;
	case SCREEN_KEYBOARD_OPEN:
		if(MergeDatabaseScreen(m_tFile).DoModal()) FilterDirty();
		break;
	case SCREEN_KEYBOARD_YES:
		//show a message box
		delete m_msgBox;
		m_msgBox=new SimpleMessageBox();

		m_msgBox->m_prompt=_("Editing level database is an experimental feature.\nPlease backup your level database first.\n\nContinue?");
		m_msgBox->m_buttons.push_back(_("Yes"));
		m_msgBox->m_buttons.push_back(_("No"));
		m_msgBox->Create();
		m_msgBox->m_nDefaultValue=0;
		m_msgBox->m_nCancelValue=1;

		//ad-hoc fix
		m_nIdleTime=0;

		break;
	default:
		break;
	}

	return -1;
}

void LevelDatabaseScreen::FilterDirty(){
	m_searchResult.clear();

	u16string s=toUTF16(m_searchFilter);

	//to lowercase
	for(int i=0,m=s.size();i<m;i++){
		if(s[i]>='A' && s[i]<='Z') s[i]+='a'-'A';
	}

	for(int ii=0,mm=m_tFile.objLevels.size();ii<mm;ii++){
		if(m_tFile.objLevels[ii].bData.empty()) continue;

		if(!s.empty()){
			u16string s2=m_tFile.objLevels[ii].sName;

			//to lowercase
			for(int i=0,m=s2.size();i<m;i++){
				if(s2[i]>='A' && s2[i]<='Z') s2[i]+='a'-'A';
			}

			//check
			if(s2.find(s)==u16string::npos) continue;
		}

		//found it
		m_searchResult.push_back(ii);
	}

	m_bDirty=true;
}

int LevelDatabaseScreen::DoModal(){
	memset(&m_tSelected,0,sizeof(m_tSelected));

	m_bDirtyOnResize=true;

	const int rightButtons[]={SCREEN_KEYBOARD_SEARCH,SCREEN_KEYBOARD_OPEN,SCREEN_KEYBOARD_YES};
	m_titleBar.m_RightButtons.assign(rightButtons,
		rightButtons+(sizeof(rightButtons)/sizeof(rightButtons[0])));

	theApp->m_objRecordMgr.LoadWholeFile(theApp->m_objRecordMgr.GetFile(),m_tFile);

	FilterDirty();

	int ret=SimpleListScreen::DoModal();

	if(theApp->m_objRecordMgr.GetFile()==NULL) theApp->m_objRecordMgr.LoadFile();

	return ret;
}

int LevelDatabaseScreen::OnMsgBoxClick(int index){
	delete m_msgBox;
	m_msgBox=NULL;

	if(index==0){
		//close main database
		theApp->m_objRecordMgr.CloseFile();
		//save changes
		if(theApp->m_objRecordMgr.SaveWholeFile(m_tFile)){
			theApp->ShowToolTip(_("Level database saved"));
			return 0;
		}else{
			theApp->ShowToolTip(_("Failed to save level database"));
		}
	}

	return -1;
}
