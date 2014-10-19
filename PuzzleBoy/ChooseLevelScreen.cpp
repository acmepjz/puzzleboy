#include "ChooseLevelScreen.h"
#include "SimpleText.h"
#include "PuzzleBoyApp.h"
#include "main.h"
#include "FileSystem.h"
#include "PuzzleBoyLevelFile.h"
#include "PuzzleBoyLevelView.h"
#include "SimpleMiscScreen.h"
#include "SimpleMessageBox.h"
#include "MyFormat.h"

#include <stdio.h>
#include <string.h>

void ChooseLevelFileScreen::OnDirty(){
	int count=m_files.size();

	ResetList();

	for(int i=0;i<count;i++){
		AddItem(m_fileDisplayName[i]);

		//update selection
		if(m_files[i]==theApp->m_sLastFile) m_nListIndex=ListCount()-1;
	}

	EnsureVisible();
}

int ChooseLevelFileScreen::OnClick(int index){
	if(!theApp->LoadFile(m_files[index])){
		theApp->ShowToolTip(str(MyFormat(_("Failed to load file %s"))<<m_files[index]));
		return -1;
	}
	return 1;
}

int ChooseLevelFileScreen::OnMsgBoxClick(int index){
	delete m_msgBox;
	m_msgBox=NULL;
	return index?0:-1;
}

int ChooseLevelFileScreen::DoModal(){
	//show a warning dialog
	if(!theApp->m_view.empty() && theApp->m_view[0]	&& theApp->m_view[0]->m_nMode==EDIT_MODE
		&& theApp->m_pDocument && theApp->m_pDocument->m_bModified)
	{
		delete m_msgBox;
		m_msgBox=CreateLevelChangedMsgBox();
	}

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
	m_titleBar.m_sTitle=_("Choose Level File");
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

	int count;

	if(m_searchFilter.empty()){
		m_titleBar.m_sTitle=_("Choose Level");
		count=theApp->m_pDocument->m_objLevels.size();
	}else{
		m_titleBar.m_sTitle=_("Search Result");
		count=m_searchResult.size();
	}

	RecreateTitleBar();

	ResetList();

	const int w1=theApp->m_nMenuTextSize*5; //160
	const int w2=88+theApp->m_nMenuTextSize*5; //256

	for(int index=0;index<count;index++){
		int i=m_searchFilter.empty()?index:m_searchResult[index];

		char s[32];

		//index
		sprintf(s,"%d",i+1);
		AddItem(s,true,0,76,DrawTextFlags::AutoSize);

		//name
		AddItem(toUTF8(theApp->m_pDocument->m_objLevels[i]->m_sLevelName),false,
			80,float(screenWidth-w2),DrawTextFlags::AutoSize);

		//size
		sprintf(s,"%dx%d",theApp->m_pDocument->m_objLevels[i]->m_nWidth,
			theApp->m_pDocument->m_objLevels[i]->m_nHeight);
		AddItem(s,false,float(screenWidth-w1));

		//get record (???)
		int st=m_bestStep[i];
		if(st>0) sprintf(s,"%d",st);
		else strcpy(s,"---");
		AddItem(s,false,float(screenWidth),0,DrawTextFlags::Right);

		//update selection
		if(i==theApp->m_nCurrentLevel) m_nListIndex=ListCount()-1;
	}

	EnsureVisible();
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
	const int rightButtons[]={SCREEN_KEYBOARD_SEARCH,SCREEN_KEYBOARD_OPEN};
	m_titleBar.m_RightButtons.assign(rightButtons,
		rightButtons+(sizeof(rightButtons)/sizeof(rightButtons[0])));
	return SimpleListScreen::DoModal();
}
