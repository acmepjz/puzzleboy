#include "ConfigScreen.h"
#include "PuzzleBoyApp.h"
#include "FileSystem.h"
#include "SimpleText.h"
#include "SimpleMiscScreen.h"
#include "MyFormat.h"
#include "main.h"

#include "include_sdl.h"
#include "include_gl.h"

enum ConfigType{
	ConfigAnimationSpeed=0,
	ConfigShowGrid,
	ConfigShowLines,
	ConfigInternationalFont,
	ConfigLanguage,
	ConfigThreadCount,
	ConfigOrientation,
	ConfigAutoSave,
	ConfigContinuousKey,
	ConfigShowFPS,
	ConfigButtonSize,
	ConfigMenuTextSize,
	ConfigMenuHeightFactor,
	ConfigEmpty_1,
	ConfigPlayerName, //player2,empty,title
	ConfigKey=ConfigPlayerName+4,
	//ConfigKey1=xx,
	//ConfigKey2=xx,
};

void ConfigScreen::OnDirty(){
	ResetList();

	u8string yesno[2];
	yesno[0]=_("No");
	yesno[1]=_("Yes");

	//animation speed
	AddItem(str(MyFormat(_("Animation Speed")+": %d%%")<<(100<<theApp->m_nAnimationSpeed)));

	//show grid
	AddItem(_("Show Grid: ")+yesno[theApp->m_bShowGrid?1:0]);

	//show lines
	AddItem(_("Show Lines: ")+yesno[theApp->m_bShowLines?1:0]);

	//international font
	AddItem(str(MyFormat(_("International Font: %s (requires restart)"))<<yesno[theApp->m_bInternationalFont?1:0]));

	//language
	{
		u8string s=theApp->m_sLocale;

		if(s.empty()) s=str(MyFormat(_("Use system language (%s)"))<<theApp->m_objGetText.m_sCurrentLocale);
		else if(s.find_first_of('?')!=u8string::npos) s=_("Disabled");

		AddItem(_("Language")+": "+s);
	}

	//thread count
	{
		u8string s;

		if(theApp->m_nThreadCount>0) s=str(MyFormat("%d")<<theApp->m_nThreadCount);
		else s=str(MyFormat(_("Automatic (%d)"))<<SDL_GetCPUCount());

		AddItem(_("Thread Count")+": "+s);
	}

	//orientation
	{
		u8string s[3];
		s[0]=_("Normal");
		s[1]=_("Horizontal Up-Down");
		s[2]=_("Vertical Up-Down");

		AddItem(_("Multiplayer Orientation")+": "+s[theApp->m_nOrientation]);
	}

	//auto save
	AddItem(_("Save Progress At Exit: ")+yesno[theApp->m_bAutoSave?1:0]);

	//continuous key
	AddItem(_("Continuous Key Event: ")+yesno[theApp->m_bContinuousKey?1:0]);

	//show FPS
	AddItem(_("Show FPS: ")+yesno[theApp->m_bShowFPS?1:0]);

	//button size
	AddItem(str(MyFormat(_("Button Size")+": %d%%")<<(theApp->m_nButtonSize*100/64)));

	//menu text size
	AddItem(str(MyFormat(_("Menu Text Size")+": %d%%")<<int(theApp->m_fMenuTextScale*100.0f)));

	//menu height
	AddItem(str(MyFormat(_("Menu Height")+": %d%%")<<(theApp->m_nMenuHeightFactor*25)));

	//empty line
	AddEmptyItem();

	//player name
	for(int i=0;i<2;i++){
		AddItem(str(MyFormat(_("Name of Player %d: "))<<(i+1))+toUTF8(theApp->m_sPlayerName[i]));
	}

	//keys
	{
		u8string keysTitle[12];
		keysTitle[0]=_("Single Player Keys:");
		keysTitle[1]=_("Multiplayer Keys (Player 1):");
		keysTitle[2]=_("Multiplayer Keys (Player 2):");
		keysTitle[3]=_("(None)");
		keysTitle[4]=_("Up: ");
		keysTitle[5]=_("Down: ");
		keysTitle[6]=_("Left: ");
		keysTitle[7]=_("Right: ");
		keysTitle[8]=_("Undo: ");
		keysTitle[9]=_("Redo: ");
		keysTitle[10]=_("Switch: ");
		keysTitle[11]=_("Restart: ");

		for(int idx=0;idx<3;idx++){
			//empty line
			AddEmptyItem();

			//title
			AddItem(keysTitle[idx]);

			for(int i=0;i<8;i++){
				u8string s;
				if(theApp->m_nKey[idx*8+i]) s=SDL_GetKeyName(theApp->m_nKey[idx*8+i]);
				else s=keysTitle[3];

				AddItem(keysTitle[i+4]+s);
			}
		}
	}
}

int ConfigScreen::OnClick(int index){
	switch(index){
	case ConfigAnimationSpeed:
		//animation speed
		{
			//init list box
			SimpleStaticListScreen screen;

			for(int i=0;i<4;i++){
				screen.m_sList.push_back(str(MyFormat("%d%%")<<(100<<i)));
			}

			screen.m_sTitle=_("Animation Speed");

			//show and get result
			int ret=screen.DoModal();
			if(ret>0 && ret-1!=theApp->m_nAnimationSpeed){
				theApp->m_nAnimationSpeed=ret-1;
				m_bConfigDirty=true;
				m_bDirty=true;
			}
		}
		break;
	case ConfigShowGrid:
		//show grid
		theApp->m_bShowGrid=!theApp->m_bShowGrid;
		m_bConfigDirty=true;
		m_bDirty=true;
		break;
	case ConfigShowLines:
		//show lines
		theApp->m_bShowLines=!theApp->m_bShowLines;
		m_bConfigDirty=true;
		m_bDirty=true;
		break;
	case ConfigInternationalFont:
		//international font
		theApp->m_bInternationalFont=!theApp->m_bInternationalFont;
		m_bConfigDirty=true;
		m_bDirty=true;
		break;
	case ConfigButtonSize:
		//button size
		{
			//init list box
			SimpleStaticListScreen screen;

			for(int i=4;i<=16;i++){
				screen.m_sList.push_back(str(MyFormat("%d%%")<<(i*25)));
			}

			screen.m_sTitle=_("Button Size");

			//show and get result
			int ret=screen.DoModal();
			if(ret>0){
				theApp->m_nButtonSize=(ret+3)*16;
				m_bConfigDirty=true;
				m_bDirty=true;
				//recreate header
				CreateTitleBarText(_("Config"));
				CreateTitleBarButtons();
			}
		}
		break;
	case ConfigLanguage:
		//language
		{
			//enumerate available locale
			std::vector<u8string> availableLocale;
			availableLocale.push_back("?");
			availableLocale.push_back("");

			std::vector<u8string> fs=enumAllFiles("data/locale/","mo");
			for(unsigned int i=0;i<fs.size();i++){
				u8string::size_type lps=fs[i].find_last_of('.');
				if(lps!=u8string::npos) availableLocale.push_back(fs[i].substr(0,lps));
				else availableLocale.push_back(fs[i]);
			}

			//init list box
			SimpleStaticListScreen screen;

			screen.m_sList.push_back(_("Disabled"));

			char buf[256];
			GNUGetText::GetSystemLocale(buf,sizeof(buf));
			screen.m_sList.push_back(str(MyFormat(_("Use system language (%s)"))<<buf));

			for(unsigned int i=2;i<availableLocale.size();i++){
				screen.m_sList.push_back(availableLocale[i]);
			}

			screen.m_sTitle=_("Language");

			//show and get result
			int ret=screen.DoModal();
			if(ret>0 && availableLocale[ret-1]!=theApp->m_sLocale){
				theApp->m_sLocale=availableLocale[ret-1];
				theApp->LoadLocale();
				m_bConfigDirty=true;
				m_bDirty=true;
			}
		}
		break;
	case ConfigThreadCount:
		//thread count
		{
			//init list box
			SimpleStaticListScreen screen;

			screen.m_sList.push_back(str(MyFormat(_("Automatic (%d)"))<<SDL_GetCPUCount()));
			for(int i=1;i<=8;i++){
				screen.m_sList.push_back(str(MyFormat("%d")<<i));
			}

			screen.m_sTitle=_("Thread Count");

			//show and get result
			int ret=screen.DoModal();
			if(ret>0 && ret-1!=theApp->m_nThreadCount){
				theApp->m_nThreadCount=ret-1;
				m_bConfigDirty=true;
				m_bDirty=true;
			}
		}
		break;
	case ConfigOrientation:
		//orientation
		{
			//init list box
			SimpleStaticListScreen screen;

			screen.m_sList.push_back(_("Normal"));
			screen.m_sList.push_back(_("Horizontal Up-Down"));
			screen.m_sList.push_back(_("Vertical Up-Down"));

			screen.m_sTitle=_("Multiplayer Orientation");

			//show and get result
			int ret=screen.DoModal();
			if(ret>0 && ret-1!=theApp->m_nOrientation){
				theApp->m_nOrientation=ret-1;
				m_bConfigDirty=true;
				m_bDirty=true;
			}
		}
		break;
	case ConfigAutoSave:
		//auto save
		theApp->m_bAutoSave=!theApp->m_bAutoSave;
		m_bConfigDirty=true;
		m_bDirty=true;
		break;
	case ConfigContinuousKey:
		//continuous key
		theApp->m_bContinuousKey=!theApp->m_bContinuousKey;
		m_bConfigDirty=true;
		m_bDirty=true;
		break;
	case ConfigShowFPS:
		//show fps
		theApp->m_bShowFPS=!theApp->m_bShowFPS;
		m_bConfigDirty=true;
		m_bDirty=true;
		break;
	case ConfigMenuTextSize:
		//menu text size
		{
			//init list box
			SimpleStaticListScreen screen;

			for(int i=4;i<=16;i++){
				screen.m_sList.push_back(str(MyFormat("%d%%")<<(i*25)));
			}

			screen.m_sTitle=_("Menu Text Size");

			//show and get result
			int ret=screen.DoModal();
			if(ret>0){
				theApp->m_nMenuTextSize=(ret+3)<<3;
				theApp->m_fMenuTextScale=theApp->m_nMenuTextSize/32.0f;
				theApp->m_nMenuHeight=(theApp->m_nMenuTextSize*theApp->m_nMenuHeightFactor)>>2;
				m_bConfigDirty=true;
				m_bDirty=true;
			}
		}
		break;
	case ConfigMenuHeightFactor:
		//menu height
		{
			//init list box
			SimpleStaticListScreen screen;

			for(int i=4;i<=16;i++){
				screen.m_sList.push_back(str(MyFormat("%d%%")<<(i*25)));
			}

			screen.m_sTitle=_("Menu Height");

			//show and get result
			int ret=screen.DoModal();
			if(ret>0){
				theApp->m_nMenuHeightFactor=(ret+3);
				theApp->m_nMenuHeight=(theApp->m_nMenuTextSize*theApp->m_nMenuHeightFactor)>>2;
				m_bConfigDirty=true;
				m_bDirty=true;
			}
		}
		break;
	case ConfigPlayerName:
	case ConfigPlayerName+1:
		//player name
		{
			int i=index-ConfigPlayerName;
			u8string s=toUTF8(theApp->m_sPlayerName[i]);

			if(SimpleInputScreen(
				str(MyFormat(_("Name of Player %d"))<<(i+1)),
				str(MyFormat(_("Please input the new name of player %d"))<<(i+1)),
				s))
			{
				theApp->m_sPlayerName[i]=toUTF16(s);
				m_bConfigDirty=true;
				m_bDirty=true;
			}
		}
		break;
	default:
		if(index>=ConfigKey && index<ConfigKey+28){
			int i=index-ConfigKey;
			int idx=i/10;
			i-=idx*10;
			if(i<8){
				i+=idx*8;
				int k=SimpleConfigKeyScreen(theApp->m_nKey[i]);
				if(k>=0){
					theApp->m_nKey[i]=k;
					m_bConfigDirty=true;
					m_bDirty=true;
				}
			}
		}
		break;
	}
	return -1;
}

int ConfigScreen::DoModal(){
	m_bConfigDirty=false;

	//show
	m_LeftButtons.push_back(SCREEN_KEYBOARD_LEFT);
	CreateTitleBarText(_("Config"));
	int ret=SimpleListScreen::DoModal();

	//save config
	if(m_bConfigDirty) theApp->SaveConfig(externalStoragePath+"/PuzzleBoy.cfg");

	return ret;
}
