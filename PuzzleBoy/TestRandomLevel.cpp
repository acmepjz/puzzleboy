#include "TestRandomLevel.h"
#include "TestSolver.h"
#include "PuzzleBoyLevelData.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int RandomTest(int width,int height,PuzzleBoyLevelData*& outputLevel,void *userData,RandomLevelCallback callback){
	struct{
		inline int operator()(int st,int blockUsed){
			return st+blockUsed*16;
		}
	}CalcScore;

	struct RandomTestData{
		PuzzleBoyLevelData *level;
		int bestStep;
		int bestScore;

		static int Compare(const void* lp1,const void* lp2){
			const RandomTestData *obj1=(const RandomTestData*)lp1;
			const RandomTestData *obj2=(const RandomTestData*)lp2;

			return (obj1->bestScore>obj2->bestScore)?-1:(obj1->bestScore<obj2->bestScore?1:0);
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

	//init pool
	for(int i=0;i<PoolSize;i++){
		PuzzleBoyLevelData &level=*(new PuzzleBoyLevelData());

		level.Create(width,height);

		memset(&(level.m_bMapData[0]),0,level.m_bMapData.size());

		//random start and end (ad-hoc) point
		int y1=int((float)height*(float)rand()/(1.0f+(float)RAND_MAX));
		level(0,y1)=PLAYER_TILE;
		int y2=int((float)height*(float)rand()/(1.0f+(float)RAND_MAX));
		level(width-1,y2)=EXIT_TILE;

		y1-=y2;
		if(y1<0) y1=-y1;
		levels[i].bestScore=levels[i].bestStep=width-1+y1;

		levels[i].level=&level;
	}

	//random rotate blocks (buggy!!!1!!)
//#define USE_TMP2
	unsigned char tmp[8][8];
#ifdef USE_TMP2
	unsigned char tmp2[8][8];
#endif

	bool bAbort=false;

	for(int r=0;r<IterationCount;r++){
		for(int levelIndex=0;levelIndex<PoolSize;levelIndex++){
			//TODO: abort
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
						if(xx>=0 && xx<8 && yy>=0 && yy<8){
							tmp2[yy][xx]=tmp2[yy][xx]/2+0x80;
						}
					}
				}
#endif
			}

			//some random tries
			for(int i=0;i<10;i++){
				int x=int((float)width*(float)rand()/(1.0f+(float)RAND_MAX));
				int y=int((float)height*(float)rand()/(1.0f+(float)RAND_MAX));
				unsigned char d[4]={};

				if((x==0 || x==width-1) && (y==0 || y==height-1)) continue;
				if(level(x,y) || tmp[y][x]) continue;

				if(y>0) d[0]=unsigned char(2.0f*(float)rand()/(1.0f+(float)RAND_MAX));
				if(x>0) d[1]=unsigned char(2.0f*(float)rand()/(1.0f+(float)RAND_MAX));
				if(y<height-1) d[2]=unsigned char(2.0f*(float)rand()/(1.0f+(float)RAND_MAX));
				if(x<width-1) d[3]=unsigned char(2.0f*(float)rand()/(1.0f+(float)RAND_MAX));

				if(d[0] && (level(x,y-1) || tmp[y-1][x])) d[0]=0;
				if(d[1] && (level(x-1,y) || tmp[y][x-1])) d[1]=0;
				if(d[2] && (level(x,y+1) || tmp[y+1][x])) d[2]=0;
				if(d[3] && (level(x+1,y) || tmp[y][x+1])) d[3]=0;

				if(d[0]==0 && d[1]==0 && d[2]==0 && d[3]==0) continue;

#ifdef USE_TMP2
				//probability test. doesn't work well
				if(unsigned char(255.0f*(float)rand()/(1.0f+(float)RAND_MAX))<tmp2[y][x]) continue;
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
					TestSolverExtendedData ed;
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
						if(xx>=0 && xx<8 && yy>=0 && yy<8){
							tmp2[yy][xx]=tmp2[yy][xx]/2+0x80;
						}
					}
				}
#endif
			}

			//remove superfluous blocks - step 1
			while(level.m_objBlocks.size()>bestCount){
				delete level.m_objBlocks.back();
				level.m_objBlocks.pop_back();
			}

			//check if nothing added
			if(bestCount==oldCount){
				//random remove something
				if(oldCount>0){
					int i=int((float)oldCount*(float)rand()/(1.0f+(float)RAND_MAX));
					delete level.m_objBlocks[i];
					level.m_objBlocks.erase(level.m_objBlocks.begin()+i);
				}

				//remove some wall
				int m=1+int(3.0f*(float)rand()/(1.0f+(float)RAND_MAX));
				for(int i=0,j=0;i<64 && j<m;i++){
					int x=int((float)width*(float)rand()/(1.0f+(float)RAND_MAX));
					int y=int((float)height*(float)rand()/(1.0f+(float)RAND_MAX));
					if(level(x,y)==WALL_TILE){
						level(x,y)=0;
						m++;
					}
				}

				/*//then add something
				int m=1+int(3.0f*(float)rand()/(1.0f+(float)RAND_MAX));
				for(int i=0,j=0;i<64 && j<m;i++){
					int x=int((float)width*(float)rand()/(1.0f+(float)RAND_MAX));
					int y=int((float)height*(float)rand()/(1.0f+(float)RAND_MAX));

					if(level(x,y) || tmp[y][x]) continue;

					level(x,y)=WALL_TILE;
					j++;
				}*/
			}else{
				//remove superfluous blocks - step 2
				for(;;){
					std::vector<PushableBlock*> tmp=level.m_objBlocks;
					int m=tmp.size();
					int bestIndex=-1;

					for(int i=0;i<m;i++){
						level.m_objBlocks.clear();
						for(int j=0;j<i;j++) level.m_objBlocks.push_back(tmp[j]);
						for(int j=i+1;j<m;j++) level.m_objBlocks.push_back(tmp[j]);

						//now try to solve it
						PuzzleBoyLevel lev(level);
						lev.StartGame();
						u8string s;
						int ret=TestSolver_SolveIt(lev,s,NULL,NULL,NULL);
						int n=s.size();

						if(ret==1 && n>=bestStep){
							bestStep=n;
							bestIndex=i;
						}
					}

					if(bestIndex==-1){
						level.m_objBlocks=tmp;
						break;
					}

					level.m_objBlocks.clear();
					for(int j=0;j<bestIndex;j++) level.m_objBlocks.push_back(tmp[j]);
					for(int j=bestIndex+1;j<m;j++) level.m_objBlocks.push_back(tmp[j]);
					delete tmp[bestIndex];
				}
			}

			//check deadlock blocks
			TestSolverExtendedData ed;
			{
				PuzzleBoyLevel lev(level);
				lev.StartGame();
				u8string s;
				TestSolver_SolveIt(lev,s,NULL,NULL,&ed);
				bestStep=s.size();
				bestScore=CalcScore(bestStep,ed.blockUsed);
			}

			if(ed.deadlockBlockCount>0){
				//remove deadlock blocks
				for(int i=level.m_objBlocks.size()-1;i>=0;i--){
					if((ed.blockStateReachable[i] & (ed.blockStateReachable[i]-1))==0
						/*&& rand()<(RAND_MAX/2)*/)
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

				//now try to remove some walls
				//TODO: use random order
				for(;;){
					bool changed=false;
					for(int j=0;j<height;j++){
						for(int i=0;i<width;i++){
							if(level(i,j)==WALL_TILE){
								level(i,j)=0;

								//now try to solve it
								PuzzleBoyLevel lev(level);
								lev.StartGame();
								u8string s;
								int ret=TestSolver_SolveIt(lev,s,NULL,NULL,NULL);
								int n=s.size();

								if(ret==1 && n>=bestStep){
									bestStep=n;
									changed=true;
								}else{
									level(i,j)=WALL_TILE;
								}
							}
						}
					}

					if(!changed) break;
				}
			}

			//over, save newly generated level
			delete levels[levelIndex+PoolSize].level;
			levels[levelIndex+PoolSize].level=&level;
			levels[levelIndex+PoolSize].bestStep=bestStep;
			levels[levelIndex+PoolSize].bestScore=bestScore;
		}

		if(bAbort) break;

		//sort levels
		qsort(levels,PoolSize*2,sizeof(RandomTestData),RandomTestData::Compare);
	}

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
