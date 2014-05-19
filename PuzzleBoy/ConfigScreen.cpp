#include "ConfigScreen.h"
#include "PuzzleBoyApp.h"
#include "FileSystem.h"
#include "SimpleText.h"
#include "SimpleMiscScreen.h"
#include "MyFormat.h"
#include "main.h"

#include "include_sdl.h"
#include "include_gl.h"

class ChooseLanguageScreen:public SimpleListScreen{
public:
	virtual void OnDirty(){
		if(m_txtList) m_txtList->clear();
		else m_txtList=new SimpleText;

		m_nListCount=0;

		m_txtList->NewStringIndex();
		m_txtList->AddString(mainFont,_("Disabled"),0,float((m_nListCount++)*32),0,32,1,DrawTextFlags::VCenter);

		{
			char buf[256];

			GNUGetText::GetSystemLocale(buf,sizeof(buf));

			m_txtList->NewStringIndex();
			m_txtList->AddString(mainFont,str(MyFormat(_("Use system language (%s)"))<<buf),
				0,float((m_nListCount++)*32),0,32,1,DrawTextFlags::VCenter);
		}

		for(unsigned int i=2;i<m_sAvailableLocale.size();i++){
			m_txtList->NewStringIndex();
			m_txtList->AddString(mainFont,m_sAvailableLocale[i],0,float((m_nListCount++)*32),0,32,1,DrawTextFlags::VCenter);
		}
	}

	virtual int OnClick(int index){
		m_sLocale=m_sAvailableLocale[index];
		return 1;
	}

	virtual int DoModal(){
		//init locale
		m_sLocale=theApp->m_sLocale;
		m_sAvailableLocale.clear();
		m_sAvailableLocale.push_back("?");
		m_sAvailableLocale.push_back("");

		{
			std::vector<u8string> fs=enumAllFiles("data/locale/","mo");
			for(unsigned int i=0;i<fs.size();i++){
				u8string::size_type lps=fs[i].find_last_of('.');
				if(lps!=u8string::npos) m_sAvailableLocale.push_back(fs[i].substr(0,lps));
				else m_sAvailableLocale.push_back(fs[i]);
			}
		}

		//show
		m_LeftButtons.push_back(SCREEN_KEYBOARD_LEFT);
		CreateTitleBarText(_("Language"));
		return SimpleListScreen::DoModal();
	}
public:
	std::vector<u8string> m_sAvailableLocale;
	u8string m_sLocale;
};

enum ConfigType{
	ConfigAnimationSpeed=0,
	ConfigShowGrid, //1
	ConfigShowLines, //2
	ConfigInternationalFont, //3
	ConfigLanguage, //4
	ConfigThreadCount, //5
	ConfigOrientation, //6
	ConfigEmpty_1, //7
	ConfigPlayerName, //player2,empty,title
	ConfigKey=ConfigPlayerName+4,
	//ConfigKey1=xx,
	//ConfigKey2=xx,
};

void ConfigScreen::OnDirty(){
	if(m_txtList) m_txtList->clear();
	else m_txtList=new SimpleText;

	m_nListCount=0;

	u8string yesno[2];
	yesno[0]=_("No");
	yesno[1]=_("Yes");

	//animation speed
	m_txtList->NewStringIndex();
	m_txtList->AddString(mainFont,str(MyFormat(_("Animation Speed: %dx"))<<(1<<theApp->m_nAnimationSpeed)),
		0,float((m_nListCount++)*32),0,32,1,DrawTextFlags::VCenter);

	//show grid
	m_txtList->NewStringIndex();
	m_txtList->AddString(mainFont,_("Show Grid: ")+yesno[theApp->m_bShowGrid?1:0],
		0,float((m_nListCount++)*32),0,32,1,DrawTextFlags::VCenter);

	//show lines
	m_txtList->NewStringIndex();
	m_txtList->AddString(mainFont,_("Show Lines: ")+yesno[theApp->m_bShowLines?1:0],
		0,float((m_nListCount++)*32),0,32,1,DrawTextFlags::VCenter);

	//international font
	m_txtList->NewStringIndex();
	m_txtList->AddString(mainFont,str(MyFormat(_("International Font: %s (reqiures restart)"))<<yesno[theApp->m_bInternationalFont?1:0]),
		0,float((m_nListCount++)*32),0,32,1,DrawTextFlags::VCenter);

	//language
	{
		u8string s=theApp->m_sLocale;

		if(s.empty()) s=str(MyFormat(_("Use system language (%s)"))<<theApp->m_objGetText.m_sCurrentLocale);
		else if(s.find_first_of('?')!=u8string::npos) s=_("Disabled");

		m_txtList->NewStringIndex();
		m_txtList->AddString(mainFont,_("Language: ")+s,
			0,float((m_nListCount++)*32),0,32,1,DrawTextFlags::VCenter);
	}

	//thread count
	{
		u8string s;

		if(theApp->m_nThreadCount>0) s=str(MyFormat("%d")<<theApp->m_nThreadCount);
		else s=str(MyFormat(_("Automatic (%d)"))<<SDL_GetCPUCount());

		m_txtList->NewStringIndex();
		m_txtList->AddString(mainFont,_("Thread Count: ")+s,
			0,float((m_nListCount++)*32),0,32,1,DrawTextFlags::VCenter);
	}

	//orientation
	{
		u8string s[3];
		s[0]=_("Normal");
		s[1]=_("Horizontal Up-Down");
		s[2]=_("Vertical Up-Down");

		m_txtList->NewStringIndex();
		m_txtList->AddString(mainFont,_("Multiplayer Orientation: ")+s[theApp->m_nOrientation],
			0,float((m_nListCount++)*32),0,32,1,DrawTextFlags::VCenter);
	}

	//empty line
	m_txtList->NewStringIndex();
	m_nListCount++;

	//player name
	for(int i=0;i<2;i++){
		m_txtList->NewStringIndex();
		m_txtList->AddString(mainFont,str(MyFormat(_("Name of Player %d: "))<<(i+1))+toUTF8(theApp->m_sPlayerName[i]),
			0,float((m_nListCount++)*32),0,32,1,DrawTextFlags::VCenter);
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
			m_txtList->NewStringIndex();
			m_nListCount++;

			//title
			m_txtList->NewStringIndex();
			m_txtList->AddString(mainFont,keysTitle[idx],
				0,float((m_nListCount++)*32),0,32,1,DrawTextFlags::VCenter);

			for(int i=0;i<8;i++){
				u8string s;
				if(theApp->m_nKey[idx*8+i]) s=SDL_GetKeyName(theApp->m_nKey[idx*8+i]);
				else s=keysTitle[3];

				m_txtList->NewStringIndex();
				m_txtList->AddString(mainFont,keysTitle[i+4]+s,
					0,float((m_nListCount++)*32),0,32,1,DrawTextFlags::VCenter);
			}
		}
	}
}

int ConfigScreen::OnClick(int index){
	switch(index){
	case ConfigAnimationSpeed:
		//animation speed
		if((++(theApp->m_nAnimationSpeed))>3) theApp->m_nAnimationSpeed=0;
		m_bConfigDirty=true;
		m_bDirty=true;
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
	case ConfigLanguage:
		//language
		{
			ChooseLanguageScreen screen;
			if(screen.DoModal() && screen.m_sLocale!=theApp->m_sLocale){
				theApp->m_sLocale=screen.m_sLocale;
				theApp->LoadLocale();
				m_bConfigDirty=true;
				m_bDirty=true;
			}
		}
		break;
	case ConfigThreadCount:
		//thread count
		if((++(theApp->m_nThreadCount))>8) theApp->m_nThreadCount=0;
		m_bConfigDirty=true;
		m_bDirty=true;
		break;
	case ConfigOrientation:
		//orientation
		if((++(theApp->m_nOrientation))>2) theApp->m_nOrientation=0;
		m_bConfigDirty=true;
		m_bDirty=true;
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
