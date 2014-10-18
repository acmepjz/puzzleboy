#include "MainMenuScreen.h"
#include "PuzzleBoyApp.h"
#include "ChooseLevelScreen.h"
#include "LevelRecordScreen.h"
#include "LevelDatabaseScreen.h"
#include "PuzzleBoyLevelFile.h"
#include "PuzzleBoyLevelView.h"
#include "PuzzleBoyLevel.h"
#include "ConfigScreen.h"
#include "RandomMapScreen.h"
#include "SimpleMiscScreen.h"
#include "SimpleProgressScreen.h"
#include "SimpleMessageBox.h"
#include "main.h"
#include "UTF8-16.h"
#include "MyFormat.h"
#include "FileSystem.h"
#include "NetworkManager.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

enum MainMenuScreenButtons{
	MainMenu_ChooseLevel,
	MainMenu_ChooseLevelFile,
	MainMenu_LevelRecord,
	MainMenu_EditLevel,
	MainMenu_Config,
	MainMenu_ExitGame,
	MainMenu_Empty,
	MainMenu_LevelSolver,
	MainMenu_RandomMap,
	MainMenu_RandomMap10,
	MainMenu_SaveTempLevelFile,
	MainMenu_StartMultiplayer,
	MainMenu_NetworkMultiplayer,
	MainMenu_LevelDatabase,
};

enum EditMenuScreenButtons{
	EditMenu_ChooseLevel,
	EditMenu_ChooseLevelFile,
	EditMenu_TestLevel,
	EditMenu_NewLevelFile,
	EditMenu_SaveLevel,
	EditMenu_Config,
	EditMenu_ExitEditor,
	EditMenu_Empty,
	EditMenu_LevelPackName,
	EditMenu_LevelName,
	EditMenu_ChangeSize,
	EditMenu_AddLevel,
	EditMenu_MoveLevel,
	EditMenu_RemoveLevel,
};

enum TestMenuScreenButtons{
	TestMenu_LevelRecord,
	TestMenu_Config,
	TestMenu_ExitTest,
	TestMenu_Empty,
	TestMenu_LevelSolver,
};

enum NetworkMainMenuScreenButtons{
	NetworkMainMenu_ChooseLevel,
	NetworkMainMenu_ChooseLevelFile,
	NetworkMainMenu_LevelRecord,
	NetworkMainMenu_Config,
	NetworkMainMenu_Disconnect,
	NetworkMainMenu_Empty,
	NetworkMainMenu_RandomMap10,
	NetworkMainMenu_SaveTempLevelFile,
	NetworkMainMenu_LevelDatabase,
};

static int SolveLevelCallback(void* userData,const LevelSolverState& progress){
	SimpleProgressScreen *progressScreen=(SimpleProgressScreen*)userData;

	progressScreen->progress=float(progress.nOpenedNodeCount)/float(progress.nAllNodeCount);
	return progressScreen->DrawAndDoEvents()?0:1;
}

static int SolveCurrentLevel(bool bTestMode){
	if(!theApp->m_view.empty()
		&& theApp->m_view[0]->m_objPlayingLevel)
	{
		PuzzleBoyLevelData *dat=theApp->m_pDocument->GetLevel(theApp->m_nCurrentLevel);
		if(dat){
			//create progress screen
			SimpleProgressScreen progressScreen;
			progressScreen.Create();

			//start solver
			PuzzleBoyLevel *lev=new PuzzleBoyLevel(*dat);
			lev->StartGame();
			u8string s;
			int ret=lev->SolveIt(s,&progressScreen,SolveLevelCallback);

			delete lev;

			//over
			progressScreen.Destroy();

			//show solution
			switch(ret){
			case 1:
				ret=LevelRecordScreen(_("Level Solver"),_("The solution is:"),s,true);
				if(ret>0){
					theApp->ApplyRecord(s,ret==2,bTestMode);
					return 0;
				}
				break;
			case 0:
				theApp->ShowToolTip(_("No solution"));
				break;
			case -1:
				theApp->ShowToolTip(_("Aborted"));
				break;
			default:
				theApp->ShowToolTip(_("Can't solve this level"));
				break;
			}
		}
	}

	return -1;
}

static bool SaveUserLevelFile(const char* format,bool useFormat){
	if(theApp->m_pDocument){
		char s0[256];
		const char* lps=NULL;

		if(useFormat){
			time_t t=time(NULL);
			tm *timeinfo=localtime(&t);
			strftime(s0,sizeof(s0),format,timeinfo);
			lps=s0;
		}else{
			lps=format;
		}

		u8string s;
		for(;;){
			char c=*(lps++);
			if(c==0) break;
			switch(c){
			case '\\':
			case '/':
			case ':':
			case '*':
			case '?':
			case '\"':
			case '<':
			case '>':
			case '|':
				continue;
			default:
				s.push_back(c);
				break;
			}
		}

		int m=s.size();
		if(m<4 || SDL_strcasecmp(s.c_str()+(m-4),".lev")) s+=".lev";

		bool ret=theApp->SaveFile(externalStoragePath+"/levels/"+s);
		if(ret){
			theApp->ShowToolTip(str(MyFormat(_("File saved to %s"))<<s));
		}else{
			theApp->ShowToolTip(str(MyFormat(_("Failed to save file %s"))<<s));
		}
	}

	return false;
}

void MainMenuScreen::OnDirty(){
	ResetList();

	AddItem(_("Choose Level"));
	AddItem(_("Choose Level File"));
	AddItem(_("Level Record"));
	AddItem(_("Edit Level"));
	AddItem(_("Config"));
	AddItem(_("Exit Game"));

	//test feature
	AddEmptyItem();

	AddItem(_("Level Solver"));
	AddItem(_("Random Map"));
	AddItem(_("Random Map")+" x10");
	AddItem(_("Save Temp Level File"));
	AddItem(_("Start Multiplayer"));
	AddItem(_("Network Multiplayer"));
	AddItem(_("Level Database"));
}

int MainMenuScreen::OnClick(int index){
	switch(index){
	case MainMenu_ChooseLevel:
		//choose level
		if(ChooseLevelScreen().DoModal()) return 1;
		break;
	case MainMenu_ChooseLevelFile:
		//choose level file
		if(ChooseLevelFileScreen().DoModal()){
			ChooseLevelScreen().DoModal();
			return 1;
		}
		break;
	case MainMenu_LevelRecord:
		//level record
		if(!theApp->m_view.empty()
			&& theApp->m_view[0]->m_objPlayingLevel)
		{
			u8string s=theApp->m_view[0]->m_objPlayingLevel->GetRecord();
			int ret=LevelRecordScreen(_("Level Record"),
				_("Copy record here or paste record\nand click 'Apply' button"),
				s);
			if(ret>0){
				theApp->ApplyRecord(s,ret==2);
				return 0;
			}
		}
		break;
	case MainMenu_EditLevel:
		//edit level
		return 3;
		break;
	case MainMenu_Config:
		//config
		ConfigScreen().DoModal();
		m_bDirty=true;
		//recreate header in case of button size changed
		RecreateTitleBar();
		//resize the game screen in case of button size changed
		m_nResizeTime++;
		break;
	case MainMenu_ExitGame:
		//exit game
		m_bRun=false;
		return 0;
		break;
	case MainMenu_LevelSolver:
		//ad-hoc solver test
		return SolveCurrentLevel(false);
		break;
	case MainMenu_RandomMap:
		//ad-hoc random level TEST
		{
			int type=RandomMapScreen().DoModal();
			if(type>0){
				PuzzleBoyLevelFile *doc=RandomMapScreen::DoRandomLevels(type,1);
				if(doc){
					delete theApp->m_pDocument;
					theApp->m_pDocument=doc;
					theApp->m_nCurrentLevel=0;
					if(theApp->m_bAutoSaveRandomMap) SaveUserLevelFile("rnd-%Y%m%d%H%M%S.lev",true);
					return 1;
				}
			}
		}
		break;
	case MainMenu_RandomMap10:
		//batch TEST
		{
			int type=RandomMapScreen().DoModal();
			if(type>0){
				PuzzleBoyLevelFile *doc=RandomMapScreen::DoRandomLevels(type,10);
				if(doc){
					delete theApp->m_pDocument;
					theApp->m_pDocument=doc;
					theApp->m_nCurrentLevel=0;
					if(theApp->m_bAutoSaveRandomMap) SaveUserLevelFile("rnd-%Y%m%d%H%M%S.lev",true);
					return 1;
				}
			}
		}
		break;
	case MainMenu_SaveTempLevelFile:
		//save temp file
		SaveUserLevelFile("tmp-%Y%m%d%H%M%S.lev",true);
		return 0;
		break;
	case MainMenu_StartMultiplayer:
		//start multiplayer!!! TEST!!
		{
			int level1,level2;
			level1=level2=theApp->m_nCurrentLevel;
			if(StartMultiplayerScreen(level1,level2)){
				theApp->m_nCurrentLevel=level1;
				return 2|(level2<<8);
			}
		}
		break;
	case MainMenu_NetworkMultiplayer:
		//network multiplayer
		{
			SimpleStaticListScreen screen;

			screen.m_sList.push_back(_("Create server"));
			screen.m_sList.push_back(_("Connect to server"));

			screen.m_titleBar.m_sTitle=_("Network Multiplayer");

			int ret=screen.DoModal();

			switch(ret){
			case 1:
				netMgr->NewSessionID();
				if(netMgr->CreateServer()){
					theApp->ShowToolTip(_("Server created successfully"));
					return 2|(theApp->m_nCurrentLevel<<8);
				}else{
					theApp->ShowToolTip(_("Can't create server"));
				}
				break;
			case 2:
				{
					u8string s;

					if(SimpleInputScreen(_("Connect to server"),_("Please input server address"),s)){
						if(netMgr->ConnectToServer(s)){
							theApp->ShowToolTip(str(MyFormat(_("Connected to %s"))<<s));
							return 0;
						}else{
							theApp->ShowToolTip(str(MyFormat(_("Can't connect to %s"))<<s));
						}
					}
				}
				break;
			}
		}
		break;
	case MainMenu_LevelDatabase:
		//level database TEST
		if(LevelDatabaseScreen().DoModal()) return 1;
		break;
	}

	return -1;
}

int MainMenuScreen::DoModal(){
	//show
	m_titleBar.m_sTitle=_("Main Menu");
	return SimpleListScreen::DoModal();
}

void EditMenuScreen::OnDirty(){
	ResetList();

	AddItem(_("Choose Level"));
	AddItem(_("Choose Level File"));
	AddItem(_("Test Level"));
	AddItem(_("New Level File"));
	AddItem(_("Save Level"));
	AddItem(_("Config"));
	AddItem(_("Exit Editor"));

	AddEmptyItem();

	AddItem(str(MyFormat(_("Level Pack Name"))(": %s")<<
		toUTF8(theApp->m_pDocument->m_sLevelPackName)));
	AddItem(str(MyFormat(_("Level Name"))(": %s")<<
		toUTF8(theApp->m_pDocument->m_objLevels[theApp->m_nCurrentLevel]->m_sLevelName)));
	AddItem(_("Change Level Size"));
	AddItem(_("Add Level"));
	AddItem(_("Move Level"));
	AddItem(_("Remove Level"));
}

int EditMenuScreen::OnClick(int index){
	switch(index){
	case EditMenu_ChooseLevel:
		//choose level
		if(ChooseLevelScreen().DoModal()) return 3;
		break;
	case EditMenu_ChooseLevelFile:
		//choose level file
		if(ChooseLevelFileScreen().DoModal()){
			ChooseLevelScreen().DoModal();
			return 3;
		}
		break;
	case EditMenu_TestLevel:
		//test level
		return 4;
		break;
	case EditMenu_NewLevelFile:
		//new level pack
		if(theApp->m_pDocument->m_bModified){
			delete m_msgBox;
			m_msgBox=CreateLevelChangedMsgBox();
			m_msgBox->m_nUserData=index;
		}else{
			return 32;
		}
		break;
	case EditMenu_SaveLevel:
		//save level
		{
			//get default file name
			u8string fileName=theApp->m_sLastFile;
			size_t n=fileName.find_last_of("\\/");
			if(n!=fileName.npos) fileName=fileName.substr(n+1);

			//show dialog
			if(!SimpleInputScreen(_("Save Level"),
				_("Please input level file name"),fileName)) break;

			if(fileName.empty()) break;
			SaveUserLevelFile(fileName.c_str(),false);
			return 0;
		}
		break;
	case EditMenu_Config:
		//config
		ConfigScreen().DoModal();
		m_bDirty=true;
		//recreate header in case of button size changed
		RecreateTitleBar();
		//resize the game screen in case of button size changed
		m_nResizeTime++;
		break;
	case EditMenu_ExitEditor:
		//exit editor
		if(theApp->m_pDocument->m_bModified){
			delete m_msgBox;
			m_msgBox=CreateLevelChangedMsgBox();
			m_msgBox->m_nUserData=index;
		}else{
			return 1;
		}
		break;
	case EditMenu_LevelPackName:
		//level pack name
		{
			u8string s=toUTF8(theApp->m_pDocument->m_sLevelPackName);
			if(!SimpleInputScreen(_("Level Pack Name"),
				_("Please input level pack name"),s)) break;
			theApp->m_pDocument->m_sLevelPackName=toUTF16(s);
			theApp->m_pDocument->SetModifiedFlag();
			m_bDirty=true;
		}
		break;
	case EditMenu_LevelName:
		//level name
		{
			u8string s=toUTF8(theApp->m_pDocument->m_objLevels[theApp->m_nCurrentLevel]->m_sLevelName);
			if(!SimpleInputScreen(_("Level Name"),
				_("Please input level name"),s)) break;
			theApp->m_pDocument->m_objLevels[theApp->m_nCurrentLevel]->m_sLevelName
				=theApp->m_view[0]->m_objPlayingLevel->m_sLevelName
				=toUTF16(s);
			theApp->m_pDocument->SetModifiedFlag();
			m_bDirty=true;
		}
		break;
	case EditMenu_ChangeSize:
		//change size
		{
			PuzzleBoyLevelFile *pDoc=theApp->m_pDocument;
			PuzzleBoyLevelData *lev=pDoc->m_objLevels[theApp->m_nCurrentLevel];

			ChangeSizeScreen frm(lev->m_nWidth,lev->m_nHeight);

			if(frm.DoModal()){
				if((frm.m_nWidth!=lev->m_nWidth || frm.m_nHeight!=lev->m_nHeight
					|| (frm.m_bPreserve && (frm.m_nXOffset!=0 || frm.m_nYOffset!=0))))
				{
					lev->Create(frm.m_nWidth,frm.m_nHeight,frm.m_bPreserve,frm.m_nXOffset,frm.m_nYOffset);
					pDoc->SetModifiedFlag();
					return 3;
				}
				return 0;
			}
		}
		break;
	case EditMenu_AddLevel:
		//add level
		{
			PuzzleBoyLevelFile *pDoc=theApp->m_pDocument;
			int idx=theApp->m_nCurrentLevel;

			PuzzleBoyLevelData *lev=new PuzzleBoyLevelData();
			lev->CreateDefault();

			if(idx>=(int)pDoc->m_objLevels.size()-1){
				pDoc->m_objLevels.push_back(lev);
				idx=pDoc->m_objLevels.size()-1;
			}else{
				if(idx<0) idx=0; else idx++;
				pDoc->m_objLevels.insert(pDoc->m_objLevels.begin()+idx,lev);
			}

			theApp->m_nCurrentLevel=idx;
			pDoc->SetModifiedFlag();
			return 3;
		}
		break;
	case EditMenu_MoveLevel:
		//move level
		{
			char s0[32];
			sprintf(s0,"%d",theApp->m_nCurrentLevel+1);
			u8string s=s0;
			if(!SimpleInputScreen(_("Move Level"),
				str(MyFormat(_("Please input the destination level number"))(" (1-%d)")
				<<int(theApp->m_pDocument->m_objLevels.size())),
				s,"01234\n56789")) break;
			int n;
			if(sscanf(s.c_str(),"%d",&n)!=1) break;
			n--;
			int m=theApp->m_pDocument->m_objLevels.size();
			if(n>=0 && n<m){
				if(n==theApp->m_nCurrentLevel) return 0;

				PuzzleBoyLevelFile *pDoc=theApp->m_pDocument;
				PuzzleBoyLevelData *lev=pDoc->m_objLevels[theApp->m_nCurrentLevel];

				if(n<theApp->m_nCurrentLevel){
					for(int i=theApp->m_nCurrentLevel;i>n;i--){
						pDoc->m_objLevels[i]=pDoc->m_objLevels[i-1];
					}
				}else{
					for(int i=theApp->m_nCurrentLevel;i<n;i++){
						pDoc->m_objLevels[i]=pDoc->m_objLevels[i+1];
					}
				}

				pDoc->m_objLevels[n]=lev;
				pDoc->SetModifiedFlag();
				theApp->m_nCurrentLevel=n;
				return 3;
			}
		}
		break;
	case EditMenu_RemoveLevel:
		//remove level
		delete m_msgBox;
		m_msgBox=new SimpleMessageBox();

		m_msgBox->m_prompt=_("Do you want to remove this level?");
		m_msgBox->m_buttons.push_back(_("Yes"));
		m_msgBox->m_buttons.push_back(_("No"));
		m_msgBox->Create();
		m_msgBox->m_nDefaultValue=0;
		m_msgBox->m_nCancelValue=1;
		m_msgBox->m_nUserData=index;

		break;
	}

	return -1;
}

int EditMenuScreen::OnMsgBoxClick(int index){
	int userData=(int)m_msgBox->m_nUserData;
	delete m_msgBox;
	m_msgBox=NULL;

	switch(userData){
	case EditMenu_NewLevelFile: //new file
		return index?-1:32;
		break;
	case EditMenu_ExitEditor: //exit
		if(index==0){
			ReloadCurrentLevel();
			return 1;
		}
		break;
	case EditMenu_RemoveLevel: //remove level
		if(index==0){
			PuzzleBoyLevelFile *pDoc=theApp->m_pDocument;
			int idx=theApp->m_nCurrentLevel;

			if(pDoc->m_objLevels.size()==1){
				pDoc->GetLevel(0)->CreateDefault();
				idx=0;
			}else if(pDoc->m_objLevels.empty()){
				PuzzleBoyLevelData *lev=new PuzzleBoyLevelData();
				lev->CreateDefault();
				pDoc->m_objLevels.push_back(lev);
				idx=0;
			}else{
				delete pDoc->m_objLevels[idx];
				pDoc->m_objLevels.erase(pDoc->m_objLevels.begin()+idx);
				if(idx>=(int)pDoc->m_objLevels.size()) idx=pDoc->m_objLevels.size()-1;
			}

			theApp->m_nCurrentLevel=idx;
			pDoc->SetModifiedFlag();
			return 3;
		}
		break;
	}

	return -1;
}

int EditMenuScreen::DoModal(){
	//save changes
	if(!theApp->m_view.empty() && theApp->m_view[0] && theApp->m_view[0]->m_nMode==EDIT_MODE){
		theApp->m_view[0]->SaveEdit();
	}

	//show
	m_titleBar.m_sTitle=_("Edit Level");
	return SimpleListScreen::DoModal();
}

void TestMenuScreen::OnDirty(){
	ResetList();

	AddItem(_("Level Record"));
	AddItem(_("Config"));
	AddItem(_("Quit to Editor"));

	//test feature
	AddEmptyItem();

	AddItem(_("Level Solver"));
}

int TestMenuScreen::OnClick(int index){
	switch(index){
	case TestMenu_LevelRecord:
		//level record
		if(!theApp->m_view.empty()
			&& theApp->m_view[0]->m_objPlayingLevel)
		{
			u8string s=theApp->m_view[0]->m_objPlayingLevel->GetRecord();
			int ret=LevelRecordScreen(_("Level Record"),
				_("Copy record here or paste record\nand click 'Apply' button"),
				s);
			if(ret>0){
				theApp->ApplyRecord(s,ret==2,true);
				return 0;
			}
		}
		break;
	case TestMenu_Config:
		//config
		ConfigScreen().DoModal();
		m_bDirty=true;
		//recreate header in case of button size changed
		RecreateTitleBar();
		//resize the game screen in case of button size changed
		m_nResizeTime++;
		break;
	case TestMenu_ExitTest:
		//back to edit mode
		return 3;
		break;
	case TestMenu_LevelSolver:
		//ad-hoc solver test
		return SolveCurrentLevel(true);
		break;
	}

	return -1;
}

int TestMenuScreen::DoModal(){
	//show
	m_titleBar.m_sTitle=_("Test Level");
	return SimpleListScreen::DoModal();
}

void NetworkMainMenuScreen::OnDirty(){
	ResetList();

	AddItem(_("Choose Level"));
	AddItem(_("Choose Level File"));
	AddItem(_("Level Record"));
	AddItem(_("Config"));
	AddItem(_("Disconnect"));

	//test feature
	AddEmptyItem();

	AddItem(_("Random Map")+" x10");
	AddItem(_("Save Temp Level File"));
	AddItem(_("Level Database"));

	//show status
	AddEmptyItem();

	std::vector<u8string> desc;
	netMgr->GetPeerDescription(desc);
	for(size_t i=0;i<desc.size();i++) AddItem(desc[i]);
}

int NetworkMainMenuScreen::OnClick(int index){
	bool updateLevel=false;

	switch(index){
	case NetworkMainMenu_ChooseLevel:
		//choose level
		if(ChooseLevelScreen().DoModal()){
			theApp->m_view[0]->m_nCurrentLevel=theApp->m_nCurrentLevel;
			theApp->m_view[0]->StartGame();
			netMgr->SendSetCurrentLevel(0x1);
			return 0;
		}
		break;
	case NetworkMainMenu_ChooseLevelFile:
		//choose level file
		if(ChooseLevelFileScreen().DoModal()){
			ChooseLevelScreen().DoModal();
			updateLevel=true;
		}
		break;
	case NetworkMainMenu_LevelRecord:
		//level record
		if(!theApp->m_view.empty()
			&& theApp->m_view[0]->m_objPlayingLevel)
		{
			u8string s=theApp->m_view[0]->m_objPlayingLevel->GetRecord();
			int ret=LevelRecordScreen(_("Level Record"),
				_("Copy record here or paste record\nand click 'Apply' button"),
				s);
			if(ret>0){
				theApp->ApplyRecord(s,ret==2); //in fact in network mode the only supported mode is animation demo
				return 0;
			}
		}
		break;
	case NetworkMainMenu_Config:
		//config
		ConfigScreen().DoModal();
		m_bDirty=true;
		//recreate header in case of button size changed
		RecreateTitleBar();
		//resize the game screen in case of button size changed
		m_nResizeTime++;
		//send change player name event
		netMgr->SendQueryLevels(0x1);
		break;
	case NetworkMainMenu_Disconnect:
		//disconnect
		netMgr->Disconnect();
		theApp->ShowToolTip(_("Disconnected"));
		return 1;
		break;
	case NetworkMainMenu_RandomMap10:
		//batch TEST
		{
			int type=RandomMapScreen().DoModal();
			if(type>0){
				netMgr->SendGeneratingRandomMap();
				PuzzleBoyLevelFile *doc=RandomMapScreen::DoRandomLevels(type,10);
				if(doc){
					delete theApp->m_pDocument;
					theApp->m_pDocument=doc;
					theApp->m_nCurrentLevel=0;
					if(theApp->m_bAutoSaveRandomMap) SaveUserLevelFile("rnd-%Y%m%d%H%M%S.lev",true);
					updateLevel=true;
				}
			}
		}
		break;
	case NetworkMainMenu_SaveTempLevelFile:
		//save temp file
		SaveUserLevelFile("tmp-%Y%m%d%H%M%S.lev",true);
		return 0;
		break;
	case NetworkMainMenu_LevelDatabase:
		//level database
		if(LevelDatabaseScreen().DoModal()) updateLevel=true;
		break;
	}

	if(updateLevel){
		theApp->StartGame(2);

		netMgr->ClearReceivedMove();
		netMgr->NewSessionID();
		netMgr->SendUpdateLevels();

		return 0;
	}

	return -1;
}

int NetworkMainMenuScreen::DoModal(){
	//show
	m_titleBar.m_sTitle=_("Network Multiplayer Main Menu");
	return SimpleListScreen::DoModal();
}
