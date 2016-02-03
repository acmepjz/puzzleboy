#include "RandomMapScreen.h"
#include "PuzzleBoyApp.h"
#include "PuzzleBoyLevelFile.h"
#include "SimpleText.h"
#include "SimpleProgressScreen.h"
#include "MT19937.h"
#include "MyFormat.h"
#include "main.h"
#include "NetworkManager.h"

#include <stdio.h>

#include "include_sdl.h"

//normal sizes
static const RandomMapSizeData RandomMapSizes[]={
	{6,6,1,-1},{6,8,1,-1},{6,10,1,-1},{8,6,1,-1},{8,8,1,-1},{10,6,1,-1},
	{5,5,2,-1},{5,7,2,-1},{7,5,2,-1},{6,6,2,-1},
	{6,6,1,NORMAL_BLOCK},{6,8,1,NORMAL_BLOCK},
	{8,6,1,NORMAL_BLOCK},
	{6,6,1,TARGET_BLOCK},{6,8,1,TARGET_BLOCK},
	{8,6,1,TARGET_BLOCK},
};
const int RandomMapSizesCount=sizeof(RandomMapSizes)/sizeof(RandomMapSizeData);

const int MaxRandomTypes=RandomMapSizesCount;

//experimental sizes
static const RandomMapSizeData RandomMapSizes2[]={
	{8,10,1,-1},{10,8,1,-1},{10,10,1,-1},
	{6,8,2,-1},{8,6,2,-1},{8,8,2,-1},
	{5,5,3,-1},{5,7,3,-1},{7,5,3,-1},{6,6,3,-1},
	{8,8,1,NORMAL_BLOCK},
	{5,5,2,NORMAL_BLOCK},{5,7,2,NORMAL_BLOCK},{7,5,2,NORMAL_BLOCK},{6,6,2,NORMAL_BLOCK},
	{8,8,1,TARGET_BLOCK},
	{5,5,2,TARGET_BLOCK},{5,7,2,TARGET_BLOCK},{7,5,2,TARGET_BLOCK},{6,6,2,TARGET_BLOCK},
	{5,5,1,TARGET_BLOCK|(1<<4)},{5,7,1,TARGET_BLOCK|(1<<4)},
	{7,5,1,TARGET_BLOCK|(1<<4)},{6,6,1,TARGET_BLOCK|(1<<4)},
};
const int RandomMapSizesCount2=sizeof(RandomMapSizes2)/sizeof(RandomMapSizeData);

static void CreateRandomMapSizeDescription(const RandomMapSizeData& size,MyFormat& fmt){
	int type=size.boxType & 0x3;
	int count=1+(size.boxType>>4);

	u8string blockNames[4];
	blockNames[0]=_("a pushable block");
	blockNames[1]=_("a target block");
	blockNames[2]=_("%d pushable blocks");
	blockNames[3]=_("%d target blocks");

	switch(type){
	case NORMAL_BLOCK:
	case TARGET_BLOCK:
		fmt(_("Rotate block with %s"));
		if(count>1){
			fmt<<str(MyFormat(blockNames[type==NORMAL_BLOCK?2:3])<<count);
		}else{
			fmt<<blockNames[type==NORMAL_BLOCK?0:1];
		}
		break;
	default:
		fmt(_("Rotate block only"));
		break;
	}

	fmt(" %dx%d ")<<size.width<<size.height;
	if(size.playerCount>1){
		fmt(_("(%d players)"))<<size.playerCount;
	}
}

void RandomMapScreen::OnDirty(){
	ResetList();

	for(int i=0;i<RandomMapSizesCount;i++){
		MyFormat fmt;
		CreateRandomMapSizeDescription(RandomMapSizes[i],fmt);
		AddItem(str(fmt));
	}
	AddItem(_("Random"));

	AddEmptyItem();

	for(int i=0;i<RandomMapSizesCount2;i++){
		MyFormat fmt;
		CreateRandomMapSizeDescription(RandomMapSizes2[i],fmt);
		AddItem(str(fmt));
	}
}

int RandomMapScreen::OnClick(int index){
	switch(index){
	case MaxRandomTypes+1:
		return -1;
	default:
		return index+1;
	}
}

int RandomMapScreen::DoModal(){
	//show
	m_titleBar.m_sTitle=_("Random Map");
	return SimpleListScreen::DoModal();
}

int RandomMapScreen::DoRandomIndirect(RandomMapSizeData size, PuzzleBoyLevelData*& outputLevel, MT19937 *rnd, void *userData, RandomLevelCallback callback){
	while (size.width == 0) {
		int type = size.height - 1;

		if (type == MaxRandomTypes || type < 0) {
			type = int((float)MaxRandomTypes*(float)rnd->Rnd() / 4294967296.0f);
		}

		int i = type;

		//normal sizes
		if (i >= 0 && i<RandomMapSizesCount) {
			size = RandomMapSizes[i];
			break;
		}
		i -= RandomMapSizesCount;

		//experimental sizes
		i -= 2;

		if (i >= 0 && i<RandomMapSizesCount2) {
			size = RandomMapSizes2[i];
			break;
		}
		i -= RandomMapSizesCount2;

		//shouldn't goes here
		printf("[DoRandom] Error: Unknown random map type: %d\n", type); //not thread safe
		return 0;
	}

	return RandomTest(
		size.width,size.height,
		size.playerCount,size.boxType,
		outputLevel,rnd,userData,callback);
}

int RandomMapScreen::DoRandom(int type,PuzzleBoyLevelData*& outputLevel,MT19937 *rnd,void *userData,RandomLevelCallback callback){
	RandomMapSizeData size = { 0, type };
	return DoRandomIndirect(size, outputLevel, rnd, userData, callback);
}

struct RandomLevelBatchProgress{
	RandomMapSizeData size;
	int nCount;
	float progress;
	volatile int *lpCurrent;
	volatile int *lpNextUnprocessed;
	volatile bool *lpbAbort;
	SDL_mutex *mutex;
	PuzzleBoyLevelFile *doc;
	SimpleProgressScreen *progressScreen; // only used when single-threaded
	MT19937 *rnd;
};

static void TestRandomLevelHeadlessProgress(float progress) {
	printf("\rGenerating random level... %0.2f%%    ", progress*100.0f);
}

static int TestRandomLevelCallback(void* userData,float progress){
	RandomLevelBatchProgress *t=(RandomLevelBatchProgress*)userData;

	if(t->mutex){
		//multi-threaded
		t->progress=progress;
		if(*(t->lpbAbort)) return 1;
	}else{
		//single-threaded
		if (netMgr) netMgr->OnTimer(true);

		float p = (float(*(t->lpCurrent)) + progress) / float(t->nCount);
		if (t->progressScreen) {
			t->progressScreen->progress = p;
			if (!t->progressScreen->DrawAndDoEvents()){
				*(t->lpbAbort) = true;
				return 1;
			}
		} else {
			TestRandomLevelHeadlessProgress(p);
		}
	}

	return 0;
}

static int TestRandomLevelThreadFunc(void* userData){
	RandomLevelBatchProgress *t=(RandomLevelBatchProgress*)userData;

	if(t->mutex) SDL_LockMutex(t->mutex);

	for(;;){
		//get next level
		int i=(*(t->lpNextUnprocessed))++;
		if(t->mutex) SDL_UnlockMutex(t->mutex);

		if(i>=t->nCount) break;

		//create random level
		PuzzleBoyLevelData *level=NULL;

		if(!RandomMapScreen::DoRandomIndirect(t->size,level,t->rnd,userData,TestRandomLevelCallback)) break;

		//save level
		if(t->mutex) SDL_LockMutex(t->mutex);

		if(t->nCount>1){
			level->m_sLevelName=toUTF16(str(MyFormat(_("Random Level %d"))<<int(t->doc->m_objLevels.size()+1)));
		}else{
			level->m_sLevelName=toUTF16(_("Random Level"));
		}

		t->doc->m_objLevels.push_back(level);

		(*(t->lpCurrent))++;
		t->progress=0.0f;
	}

	// Set mutex to NULL to indicate we are finished
	t->mutex = NULL;

	return 0;
}

PuzzleBoyLevelFile* RandomMapScreen::DoRandomLevels(int type, int levelCount, bool headless){
	RandomMapSizeData size = { 0, type };
	return DoRandomLevelsIndirect(size, levelCount, headless);
}

PuzzleBoyLevelFile* RandomMapScreen::DoRandomLevelsIndirect(const RandomMapSizeData& size, int levelCount, bool headless){
	if(levelCount<=0) return NULL;

#ifdef RANDOM_MAP_PROFILING
	Uint32 t=SDL_GetTicks();
#endif

	//determine thread count
	int threadCount=theApp->m_nThreadCount;
	if(threadCount<=0) threadCount=SDL_GetCPUCount();
	if(threadCount<1) threadCount=1;
	else if(threadCount>8) threadCount=8;
	if(threadCount>levelCount) threadCount=levelCount;

	//create levels
	PuzzleBoyLevelFile* doc=new PuzzleBoyLevelFile();
	doc->m_sLevelPackName=toUTF16(_("Random Level"));

	//create progress screen
	SimpleProgressScreen progressScreen;
	if(!headless) progressScreen.Create();

	SDL_mutex *mutex=NULL;
	if(threadCount>1) mutex=SDL_CreateMutex();

	volatile int nCurrent=0;
	volatile int nNextUnprocessed=0;
	volatile bool bAbort=false;

	RandomLevelBatchProgress prog[8];
	for(int i=0;i<threadCount;i++){
		prog[i].size=size;
		prog[i].nCount=levelCount;
		prog[i].progress=0.0f;
		prog[i].lpCurrent=&nCurrent;
		prog[i].lpNextUnprocessed=&nNextUnprocessed;
		prog[i].lpbAbort=&bAbort;
		prog[i].mutex=mutex;
		prog[i].doc=doc;
		prog[i].progressScreen = headless ? NULL : &progressScreen;

		MT19937 *rnd=new MT19937;
		unsigned int seed[16];
		for(int j=0;j<16;j++){
			seed[j]=theApp->m_objMainRnd.Rnd();
		}
		rnd->Init(seed,sizeof(seed)/sizeof(unsigned int));

		prog[i].rnd=rnd;
	}

	//start thread
	if(threadCount>1){
		SDL_Thread *thread[8];
		for(int i=0;i<threadCount;i++){
			thread[i]=SDL_CreateThread(TestRandomLevelThreadFunc,NULL,&(prog[i]));
		}

		for(;;){
			SDL_Delay(30);

			if(netMgr) netMgr->OnTimer(true);

			if(nCurrent>=levelCount) break;

			int finished = 0;
			float progress=(float)nCurrent;
			for(int i=0;i<threadCount;i++){
				progress += prog[i].progress;
				if (prog[i].mutex == NULL) finished++;
			}
			if (finished >= threadCount) break;

			float p = progress / (float)levelCount;
			if (headless) {
				TestRandomLevelHeadlessProgress(p);
			} else {
				progressScreen.progress = p;
				if (!progressScreen.DrawAndDoEvents()){
					bAbort = true;
					break;
				}
			}
		}

		for(int i=0;i<threadCount;i++){
			SDL_WaitThread(thread[i],NULL);
		}
	}else{
		TestRandomLevelThreadFunc(&(prog[0]));
	}

	//over
	if(mutex) SDL_DestroyMutex(mutex);

	for(int i=0;i<threadCount;i++){
		delete prog[i].rnd;
	}

#ifdef RANDOM_MAP_PROFILING
	//print statistics
	printf("[DoRandomLevels] Create %d random level(s) in %dms\n",doc->m_objLevels.size(),SDL_GetTicks()-t);
#endif

	if (headless) {
		printf("\rGenerating random level... Done.    \n");
	}

	if(doc->m_objLevels.empty()){
		delete doc;
		return NULL;
	}else{
		return doc;
	}
}
