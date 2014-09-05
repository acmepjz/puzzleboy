#include "TestRandomLevel.h"
#include "TestSolver.h"
#include "PuzzleBoyLevelData.h"
#include "MT19937.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>

#ifdef RANDOM_MAP_PROFILING
#include <SDL.h>
#endif

static const int MAX_WIDTH=16;
static const int MAX_HEIGHT=16;

inline int CalcScore(int st,int blockUsed){
	return st+blockUsed*16;
}

int RandomTest(int width,int height,PuzzleBoyLevelData*& outputLevel,MT19937* rnd,void *userData,RandomLevelCallback callback){
	struct RandomTestData{
		PuzzleBoyLevelData *level;
		int bestStep;
		int bestScore;

		static bool Compare(const RandomTestData& obj1,const RandomTestData& obj2){
			return (obj1.bestScore>obj2.bestScore);
		}
	};

	//fake genetic algorithm
#ifdef _DEBUG
	const int PoolSize=8;
#else
	const int PoolSize=16;
#endif
	const int IterationCount=20;
	RandomTestData levels[PoolSize*2]={};

	outputLevel=NULL;

#ifdef RANDOM_MAP_PROFILING
	//profiling
	int tt0=0,tt1=0,tt2=0;
#endif

	//init pool
	for(int i=0;i<PoolSize;i++){
		PuzzleBoyLevelData &level=*(new PuzzleBoyLevelData());

		level.Create(width,height);

		memset(&(level.m_bMapData[0]),0,level.m_bMapData.size());

		//random start and end (ad-hoc) point
		int y1=int((float)height*(float)rnd->Rnd()/4294967296.0f);
		level(0,y1)=PLAYER_TILE;
		int y2=int((float)height*(float)rnd->Rnd()/4294967296.0f);
		level(width-1,y2)=EXIT_TILE;

		y1-=y2;
		if(y1<0) y1=-y1;
		levels[i].bestScore=levels[i].bestStep=width-1+y1;

		levels[i].level=&level;
	}

	//random rotate blocks (buggy!!!1!!)
//#define USE_TMP2
	unsigned char tmp[MAX_HEIGHT][MAX_WIDTH];
#ifdef USE_TMP2
	unsigned char tmp2[MAX_HEIGHT][MAX_WIDTH];
#endif

	bool bAbort=false;

	for(int r=0;r<IterationCount;r++){
		for(int levelIndex=0;levelIndex<PoolSize;levelIndex++){
			//check abort
			if(callback && callback(userData,(r*PoolSize+levelIndex)/float(IterationCount*PoolSize))){
				bAbort=true;
				break;
			}

			PuzzleBoyLevelData &level=*(new PuzzleBoyLevelData(*(levels[levelIndex].level)));
			int bestStep=levels[levelIndex].bestStep;
			int bestScore=levels[levelIndex].bestScore;

			size_t oldCount=level.m_objBlocks.size();
			size_t bestCount=oldCount;

			//re-init hit test area
			memset(tmp,0,sizeof(tmp));
#ifdef USE_TMP2
			memset(tmp2,0,sizeof(tmp2));
#endif
			for(size_t i=0;i<bestCount;i++){
				PushableBlock *block=level.m_objBlocks[i];
				int x=block->m_x;
				int y=block->m_y;
				tmp[y][x]=1;
				if(block->m_bData[0]) tmp[y-1][x]=1;
				if(block->m_bData[1]) tmp[y][x-1]=1;
				if(block->m_bData[2]) tmp[y+1][x]=1;
				if(block->m_bData[3]) tmp[y][x+1]=1;

#ifdef USE_TMP2
				for(int jj=-1;jj<=1;jj++){
					for(int ii=-1;ii<=1;ii++){
						int xx=x+ii,yy=y+jj;
						if(xx>=0 && xx<MAX_WIDTH && yy>=0 && yy<MAX_HEIGHT){
							tmp2[yy][xx]=tmp2[yy][xx]/2+0x80;
						}
					}
				}
#endif
			}

			TestSolverExtendedData ed;

#ifdef RANDOM_MAP_PROFILING
			int ttt=SDL_GetTicks();
#endif

			//some random tries
			for(int i=0;i<10;i++){
				int x=int((float)width*(float)rnd->Rnd()/4294967296.0f);
				int y=int((float)height*(float)rnd->Rnd()/4294967296.0f);
				unsigned char d[4]={};

				if((x==0 || x==width-1) && (y==0 || y==height-1)) continue;
				if(level(x,y) || tmp[y][x]) continue;

//#define THRESHOLD 1717986918U
#define THRESHOLD 0x80000000U

				if(y>0) d[0]=(rnd->Rnd()>=THRESHOLD)?1:0;
				if(x>0) d[1]=(rnd->Rnd()>=THRESHOLD)?1:0;
				if(y<height-1) d[2]=(rnd->Rnd()>=THRESHOLD)?1:0;
				if(x<width-1) d[3]=(rnd->Rnd()>=THRESHOLD)?1:0;

				if(d[0] && (level(x,y-1) || tmp[y-1][x])) d[0]=0;
				if(d[1] && (level(x-1,y) || tmp[y][x-1])) d[1]=0;
				if(d[2] && (level(x,y+1) || tmp[y+1][x])) d[2]=0;
				if(d[3] && (level(x+1,y) || tmp[y][x+1])) d[3]=0;

				if(d[0]==0 && d[1]==0 && d[2]==0 && d[3]==0) continue;

#ifdef USE_TMP2
				//probability test. doesn't work well
				if(unsigned char(255.0f*(float)rnd->Rnd()/4294967296.0f)<tmp2[y][x]) continue;
#endif

				//OK, we find a position to place rotate block
				PushableBlock *block=new PushableBlock();
				block->CreateSingle(ROTATE_BLOCK,x,y);
				memcpy(&(block->m_bData[0]),d,4);
				level.m_objBlocks.push_back(block);

				//now try to solve it
				{
					PuzzleBoyLevel lev(level);
					lev.StartGame();
					u8string s;
					int ret=TestSolver_SolveIt(lev,s,NULL,NULL,&ed);

					if(ret==1){
						int n=s.size();
						int sc=CalcScore(n,ed.blockUsed);
						if(sc>bestScore){
							bestStep=n;
							bestScore=sc;
							bestCount=level.m_objBlocks.size();
						}
						//debug
#ifdef _DEBUG
						printf("Debug: blockUsed=%d\n",ed.blockUsed);
#endif
					}else{
						level.m_objBlocks.pop_back();
						delete block;
						continue;
					}
				}

				//update hit test area
				tmp[y][x]=1;
				if(d[0]) tmp[y-1][x]=1;
				if(d[1]) tmp[y][x-1]=1;
				if(d[2]) tmp[y+1][x]=1;
				if(d[3]) tmp[y][x+1]=1;

#ifdef USE_TMP2
				for(int jj=-1;jj<=1;jj++){
					for(int ii=-1;ii<=1;ii++){
						int xx=x+ii,yy=y+jj;
						if(xx>=0 && xx<MAX_WIDTH && yy>=0 && yy<MAX_HEIGHT){
							tmp2[yy][xx]=tmp2[yy][xx]/2+0x80;
						}
					}
				}
#endif
			}

#ifdef RANDOM_MAP_PROFILING
			tt0+=SDL_GetTicks()-ttt;
#endif

			//remove superfluous blocks - step 1
			while(level.m_objBlocks.size()>bestCount){
				delete level.m_objBlocks.back();
				level.m_objBlocks.pop_back();
			}

			bool addWalls=false;

			//check if nothing added
			if(bestCount==oldCount){
				//random remove something
				if(oldCount>0){
					int i=int((float)oldCount*(float)rnd->Rnd()/4294967296.0f);
					delete level.m_objBlocks[i];
					level.m_objBlocks.erase(level.m_objBlocks.begin()+i);
				}

				//remove some wall
				int m=1+int(3.0f*(float)rnd->Rnd()/4294967296.0f);
				for(int i=0,j=0;i<64 && j<m;i++){
					int x=int((float)width*(float)rnd->Rnd()/4294967296.0f);
					int y=int((float)height*(float)rnd->Rnd()/4294967296.0f);
					if(level(x,y)==WALL_TILE){
						level(x,y)=0;
						m++;
					}
				}

				/*//then add something
				int m=1+int(3.0f*(float)rnd->Rnd()/4294967296.0f);
				for(int i=0,j=0;i<64 && j<m;i++){
					int x=int((float)width*(float)rnd->Rnd()/4294967296.0f);
					int y=int((float)height*(float)rnd->Rnd()/4294967296.0f);

					if(level(x,y) || tmp[y][x]) continue;

					level(x,y)=WALL_TILE;
					j++;
				}*/
			}else{
				//remove superfluous blocks - step 2

#ifdef RANDOM_MAP_PROFILING
				int ttt=SDL_GetTicks();
#endif

				//0=unknown 1=removed 2=can't remove
				const int m=level.m_objBlocks.size();
				std::vector<unsigned char> removable(m,0);
				std::vector<PushableBlock*> tmp=level.m_objBlocks;

				//experiment ???????? (2)
#ifdef USE_SOLUTION_REACHABLE
				for(int i=0;i<m;i++){
					int x=tmp[i]->m_x,y=tmp[i]->m_y;
					int count=0;

					if(tmp[i]->m_bData[0] && y>0 && ed.solutionReachable[(y-1)*16+x]) count++;
					if(tmp[i]->m_bData[1] && x>0 && ed.solutionReachable[y*16+(x-1)]) count++;
					if(tmp[i]->m_bData[2] && y<height-1 && ed.solutionReachable[(y+1)*16+x]) count++;
					if(tmp[i]->m_bData[3] && x<width-1 && ed.solutionReachable[y*16+(x+1)]) count++;

					if(count>=2) removable[i]=2; //???
				}
#endif

				for(;;){
					//random shuffle
					for(int i=0;i<m-1;i++){
						int j=i+int(float(m-i)*(float)rnd->Rnd()/4294967296.0f);
						if(j>i){
							std::swap(removable[i],removable[j]);
							std::swap(tmp[i],tmp[j]);
						}
					}

					int bestIndex=-1;
					int threshold=bestStep; //bestStep-2; //FIXME: arbitrary

					for(int i=0;i<m;i++){
						//try to remove position i
						if(removable[i]!=0) continue; //can't remove it or already removed

						level.m_objBlocks.clear();
						for(int j=0;j<m;j++){
							if(removable[j]!=1 && j!=i) level.m_objBlocks.push_back(tmp[j]);
						}

						//now try to solve it
						PuzzleBoyLevel lev(level);
						lev.StartGame();
						u8string s;
						int ret=TestSolver_SolveIt(lev,s,NULL,NULL,NULL);
						int n=s.size();

						if(ret==1 && n>=bestStep){
							bestStep=n;
							bestIndex=i;
							break; //experiment ???????? (1)
						}
						if(ret<1 || n<threshold){
							//experiment ???????? (3): try to replace it by wall
							/*PushableBlock *block=tmp[i];
							int x=block->m_x,y=block->m_y;

							lev(x,y)=WALL_TILE;
							if(block->m_bData[0]) lev(x,y-1)=WALL_TILE;
							if(block->m_bData[1]) lev(x-1,y)=WALL_TILE;
							if(block->m_bData[2]) lev(x,y+1)=WALL_TILE;
							if(block->m_bData[3]) lev(x+1,y)=WALL_TILE;

							lev.StartGame();
							int ret=TestSolver_SolveIt(lev,s,NULL,NULL,NULL);
							int n=s.size();

							if(ret==1 && n>=bestStep){
								level(x,y)=WALL_TILE;
								if(block->m_bData[0]) level(x,y-1)=WALL_TILE;
								if(block->m_bData[1]) level(x-1,y)=WALL_TILE;
								if(block->m_bData[2]) level(x,y+1)=WALL_TILE;
								if(block->m_bData[3]) level(x+1,y)=WALL_TILE;
								addWalls=true;
								bestStep=n;
								bestIndex=i;
								break;
							}*/

							//can't remove it because the step get fewer
							removable[i]=2;
						}
					}

					if(bestIndex==-1) break;

					//remove it
					removable[bestIndex]=1;
				}

				//apply changes
				level.m_objBlocks.clear();
				for(int j=0;j<m;j++){
					if(removable[j]==1){
						delete tmp[j];
					}else{
						level.m_objBlocks.push_back(tmp[j]);
					}
				}

#ifdef RANDOM_MAP_PROFILING
				tt1+=SDL_GetTicks()-ttt;
#endif
			}

			//check deadlock blocks
			{
				PuzzleBoyLevel lev(level);
				lev.StartGame();
				u8string s;
				TestSolver_SolveIt(lev,s,NULL,NULL,&ed);
				bestStep=s.size();
				bestScore=CalcScore(bestStep,ed.blockUsed);
			}

			if(ed.deadlockBlockCount>0 || addWalls){
				//remove deadlock blocks
				for(int i=level.m_objBlocks.size()-1;i>=0;i--){
					if((ed.blockStateReachable[i] & (ed.blockStateReachable[i]-1))==0
						/*&& rnd->Rnd()<2147483648.0f*/)
					{
						PushableBlock *block=level.m_objBlocks[i];
						int x=block->m_x,y=block->m_y;

						level(x,y)=WALL_TILE;
						if(block->m_bData[0]) level(x,y-1)=WALL_TILE;
						if(block->m_bData[1]) level(x-1,y)=WALL_TILE;
						if(block->m_bData[2]) level(x,y+1)=WALL_TILE;
						if(block->m_bData[3]) level(x+1,y)=WALL_TILE;

						delete block;
						level.m_objBlocks.erase(level.m_objBlocks.begin()+i);
					}
				}

#ifdef RANDOM_MAP_PROFILING
				int ttt=SDL_GetTicks();
#endif

				//now try to remove some walls
				std::vector<unsigned char> walls;
				for(int j=0;j<height;j++){
					for(int i=0;i<width;i++){
						if(level(i,j)==WALL_TILE){
							walls.push_back((j<<4)|i);
						}
					}
				}

				bool changed;
				do{
					changed=false;
					const int m=walls.size();
					if(m>0){
						//random shuffle
						for(int i=0;i<m-1;i++){
							int j=i+int(float(m-i)*(float)rnd->Rnd()/4294967296.0f);
							if(j>i) std::swap(walls[i],walls[j]);
						}

						int threshold=bestStep-4; //FIXME: arbitrary

						for(int i=m-1;i>=0;i--){
							int idx=(walls[i]>>4)*level.m_nWidth+(walls[i]&0xF);
							level[idx]=0;

							//now try to solve it
							PuzzleBoyLevel lev(level);
							lev.StartGame();
							u8string s;
							int ret=TestSolver_SolveIt(lev,s,NULL,NULL,NULL);
							int n=s.size();

							if(ret==1 && n>=bestStep){
								bestStep=n;
								changed=true;
								walls.erase(walls.begin()+i);
							}else{
								level[idx]=WALL_TILE;
								if(n<threshold) walls.erase(walls.begin()+i); //can't remove this wall
							}
						}
					}
				}while(changed);

#ifdef RANDOM_MAP_PROFILING
				tt2+=SDL_GetTicks()-ttt;
#endif
			}

			//over, save newly generated level
			delete levels[levelIndex+PoolSize].level;
			levels[levelIndex+PoolSize].level=&level;
			levels[levelIndex+PoolSize].bestStep=bestStep;
			levels[levelIndex+PoolSize].bestScore=bestScore;
		}

		if(bAbort) break;

		//sort levels
		std::stable_sort(levels,levels+(PoolSize*2),RandomTestData::Compare);
	}

#ifdef RANDOM_MAP_PROFILING
	//debug!!!
	printf("random add blocks %dms\nremove superfluous blocks %dms\nremove some walls %dms\n",tt0,tt1,tt2);
#endif

	//get return value
	if(!bAbort){
		outputLevel=new PuzzleBoyLevelData;
		outputLevel->Create(width+1,height);
		for(int i=0,m=levels[0].level->m_objBlocks.size();i<m;i++){
			outputLevel->m_objBlocks.push_back(new PushableBlock(*(levels[0].level->m_objBlocks[i])));
		}
		for(int j=0;j<height;j++){
			for(int i=0;i<width;i++){
				unsigned char c=(*levels[0].level)(i,j);
				if(i==width-1){
					if(c==EXIT_TILE){
						c=0;
						(*outputLevel)(width,j)=EXIT_TILE;
					}else{
						(*outputLevel)(width,j)=WALL_TILE;
					}
				}
				(*outputLevel)(i,j)=c;
			}
		}
	}

	//destroy pool
	for(int i=0;i<PoolSize*2;i++){
		delete levels[i].level;
	}

	//over
	if(bAbort) return 0;
	return levels[0].bestStep+1;
}
