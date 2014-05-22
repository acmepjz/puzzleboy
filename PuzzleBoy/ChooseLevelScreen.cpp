#include "ChooseLevelScreen.h"
#include "SimpleText.h"
#include "PuzzleBoyApp.h"
#include "main.h"
#include "FileSystem.h"
#include "PuzzleBoyLevelFile.h"
#include "SimpleMiscScreen.h"

#include <stdio.h>
#include <string.h>

void ChooseLevelFileScreen::OnDirty(){
	m_nListCount=m_files.size();

	if(m_txtList) m_txtList->clear();
	else m_txtList=new SimpleText;

	for(int i=0;i<m_nListCount;i++){
		m_txtList->NewStringIndex();
		m_txtList->AddString(mainFont,m_fileDisplayName[i],0,float(i*32),0,32,1,DrawTextFlags::VCenter);
	}
}

int ChooseLevelFileScreen::OnClick(int index){
	if(!theApp->LoadFile(m_files[index])){
		printf("[ChooseLevelFileScreen] Failed to load file %s\n",m_files[index].c_str());
		return -1;
	}
	return 1;
}

int ChooseLevelFileScreen::DoModal(){
	//enum internal files
	{
		u8string fn="data/levels/";
		std::vector<u8string> fs=enumAllFiles(fn,"lev");
		for(size_t i=0;i<fs.size();i++){
			m_files.push_back(fn+fs[i]);
			m_fileDisplayName.push_back(fs[i]);
		}
	}

	//enum external files
	{
		u8string fn=externalStoragePath+"/levels/";
		std::vector<u8string> fs=enumAllFiles(fn,"lev");
		for(size_t i=0;i<fs.size();i++){
			m_files.push_back(fn+fs[i]);
			m_fileDisplayName.push_back(_("User level: ")+fs[i]);
		}
	}

	//show
	m_LeftButtons.push_back(SCREEN_KEYBOARD_LEFT);
	CreateTitleBarText(_("Choose Level File"));
	return SimpleListScreen::DoModal();
}


void ChooseLevelScreen::OnDirty(){
	if(m_bestStep.empty()){
		int m=theApp->m_pDocument->m_objLevels.size();
		for(int i=0;i<m;i++){
			int st=theApp->m_objRecordMgr.FindLevelAndRecord(*(theApp->m_pDocument->m_objLevels[i]),NULL,NULL,NULL,NULL);
			m_bestStep.push_back(st);
		}
	}

	if(m_searchFilter.empty()){
		CreateTitleBarText(_("Choose Level"));
		m_nListCount=theApp->m_pDocument->m_objLevels.size();
	}else{
		CreateTitleBarText(_("Search Result"));
		m_nListCount=m_searchResult.size();
	}

	if(m_txtList) m_txtList->clear();
	else m_txtList=new SimpleText;

	for(int index=0;index<m_nListCount;index++){
		int i=m_searchFilter.empty()?index:m_searchResult[index];

		char s[32];
		m_txtList->NewStringIndex();
		sprintf(s,"%d",i+1);
		m_txtList->AddString(mainFont,s,0,float(index*32),0,32,1,DrawTextFlags::VCenter);
		m_txtList->AddString(mainFont,toUTF8(theApp->m_pDocument->m_objLevels[i]->m_sLevelName),
			80,float(index*32),float(screenWidth-256),32,1,DrawTextFlags::VCenter | DrawTextFlags::AutoSize);
		sprintf(s,"%dx%d",theApp->m_pDocument->m_objLevels[i]->m_nWidth,
			theApp->m_pDocument->m_objLevels[i]->m_nHeight);
		m_txtList->AddString(mainFont,s,float(screenWidth-160),float(index*32),0,32,1,DrawTextFlags::VCenter);

		//get record (???)
		int st=m_bestStep[i];
		if(st>0) sprintf(s,"%d",st);
		else strcpy(s,"---");
		m_txtList->AddString(mainFont,s,float(screenWidth),float(index*32),0,32,1,
			DrawTextFlags::Right | DrawTextFlags::VCenter);
	}
}

int ChooseLevelScreen::OnClick(int index){
	if(!m_searchFilter.empty()) index=m_searchResult[index];
	theApp->m_nCurrentLevel=index;
	return 1;
}

int ChooseLevelScreen::OnTitleBarButtonClick(int index){
	switch(index){
	case SCREEN_KEYBOARD_LEFT:
		if(m_searchFilter.empty()) break;
		//clear search filter and display again
		m_searchFilter.clear();
		m_searchResult.clear();
		m_bDirty=true;
		return -1;
		break;
	case SCREEN_KEYBOARD_OPEN:
		if(ChooseLevelFileScreen().DoModal()){
			m_nReturnValue=1;
			m_searchFilter.clear();
			m_bestStep.clear();
			m_bDirty=true;
		}
		return -1;
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

			m_searchResult.clear();

			if(!m_searchFilter.empty()){
				u16string s=toUTF16(m_searchFilter);

				//to lowercase
				for(int i=0,m=s.size();i<m;i++){
					if(s[i]>='A' && s[i]<='Z') s[i]+='a'-'A';
				}

				for(int ii=0,mm=theApp->m_pDocument->m_objLevels.size();ii<mm;ii++){
					u16string s2=theApp->m_pDocument->m_objLevels[ii]->m_sLevelName;

					//to lowercase
					for(int i=0,m=s2.size();i<m;i++){
						if(s2[i]>='A' && s2[i]<='Z') s2[i]+='a'-'A';
					}

					//check
					if(s2.find(s)==u16string::npos) continue;

					//found it
					m_searchResult.push_back(ii);
				}
			}

			m_bDirty=true;
		}
		return -1;
		break;
	}

	return m_nReturnValue;
}

int ChooseLevelScreen::DoModal(){
	m_bDirtyOnResize=true;
	m_LeftButtons.push_back(SCREEN_KEYBOARD_LEFT);
	m_RightButtons.push_back(SCREEN_KEYBOARD_SEARCH);
	m_RightButtons.push_back(SCREEN_KEYBOARD_OPEN);
	return SimpleListScreen::DoModal();
}
