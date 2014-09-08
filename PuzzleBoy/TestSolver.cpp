#include "TestSolver.h"
#include "MyFormat.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <map>
#include <vector>

#include <SDL.h>

//test solver for level with rotate blocks only

struct TestRotateBlock{
	unsigned char pos;
	unsigned char type;
	//type=0x0 1000 0100 0010 0001
	//type=0x4 1100 0110 0011 1001
	//type=0x8 1010 0101
	//type=0xC 1110 0111 1011 1101
	//type=0xA 1111
	unsigned char shift;
	unsigned char mask; //0(type=0xA) or 1(type=0x8) or 3(else)
};

#define FORCE_64BIT_SOLVER_STATE

#ifdef FORCE_64BIT_SOLVER_STATE
typedef unsigned long long TestSolverStateType;
#else
typedef uintptr_t TestSolverStateType;
#endif

struct TestSolverNode{
	int parent;
	TestSolverStateType state;
};

static const unsigned char HitTestLUT[16]={
	1,2,4,8,
	3,6,12,9,
	5,10,15,0,
	7,14,13,11,
};

static const char dirLUT[4]={-16,-1,16,1};
static const char dirName[4]={'W','A','S','D'};

//assume key type is integral
template<int S,class K,class T>
class SimpleHashMap{
public:
	static const int map_shift=S;
	static const int map_count=1<<S;
	static const int map_mask=map_count-1;
	static int hash(const K& k){
		unsigned int kk=(unsigned int)k;
		return (((kk>>3)^(kk>>11))+kk)&map_mask;
	}
	T& operator[](const K& k){
		return maps[hash(k)][k];
	}
	bool find(const K& k){
		int index=hash(k);
		return maps[index].find(k)!=maps[index].end();
	}
	bool end(){
		return false;
	}
private:
	std::map<K,T> maps[map_count];
};

//return value: bit0-5: index, bit6-7: direction, bit8-11: current type
//-1=none
static int TestSolver_HitTestForBlocks(const unsigned char mapAdjacency[256][4],const TestRotateBlock* blocks,TestSolverStateType state,unsigned char pos){
	for(int i=0;i<4;i++){
		unsigned char c=mapAdjacency[pos][i];
		if(c==0xFF) break;

		int idx=c&0x3F;
		int ii=blocks[idx].type;
		if(blocks[idx].mask) ii+=(state>>blocks[idx].shift) & blocks[idx].mask;
		if(HitTestLUT[ii] & (1<<(c>>6))) return (ii<<8)|c;
	}

	return -1;
}

static bool TestSolver_CalculateAdjacancyData(unsigned char mapAdjacency[256][4],const unsigned char* mapData,const TestRotateBlock* blocks,int blockCount){
	memset(mapAdjacency,-1,1024);

	unsigned char counts[256]={};

	for(int ii=0;ii<blockCount;ii++){
		for(int d=0;d<4;d++){
			unsigned char idx=blocks[ii].pos+dirLUT[d];
			unsigned char count=counts[idx];
			switch(mapData[idx]){
			case FLOOR_TILE: case EMPTY_TILE: case EXIT_TILE:
				if(count>=4){
					printf("[TestSolver] Error: Too many adjacent blocks\n");
					return false;
				}
				mapAdjacency[idx][count]=ii|(d<<6);
				counts[idx]++;
				break;
			}
		}
	}

	return true;
}

static TestSolverStateType TestSolver_SortPlayerPosition(int playerCount,unsigned char playerSize,TestSolverStateType pos){
	switch(playerCount){
	case 2:
		{
			unsigned int mask=(1<<playerSize)-1;
			unsigned int p1=pos&mask;
			unsigned int p2=(pos>>playerSize)&mask;

			if(p1>p2) return (p1<<playerSize)|p2|(pos&~((TestSolverStateType(1)<<(playerSize*2))-1));
		}
		break;
	case 3:
		{
			unsigned int mask=(1<<playerSize)-1;
			unsigned int p1=pos&mask;
			unsigned int p2=(pos>>playerSize)&mask;
			unsigned int p3=(pos>>(playerSize*2))&mask;

			if(p1>p2){
				if(p1>p3){
					std::swap(p1,p3); //p1 biggest, swap to p3
					if(p1>p2) std::swap(p1,p2);
				}else{ //p1<=p3
					std::swap(p1,p2);
				}
			}else{ //p1<=p2
				if(p2>p3){
					std::swap(p2,p3); //p2 biggest, swap to p3
					if(p1>p2) std::swap(p1,p2);
				}else{ //p2<=p3
					break; //order is already correct
				}
			}

			return p1|(p2<<playerSize)|(p3<<(playerSize*2))
				|(pos&~((TestSolverStateType(1)<<(playerSize*3))-1));
		}
		break;
	default:
		break;
	}

	return pos;
}

int TestSolver_SolveIt(const PuzzleBoyLevel& level,u8string& rec,void* userData,LevelSolverCallback callback,TestSolverExtendedData *ed){
	//DEBGU
	int ttt=SDL_GetTicks();

	//type|(dir<<4)
	const unsigned char LUT1[16]={
		0xFF,0x00,0x10,0x04, //0000 1000 0100 1100
		0x20,0x08,0x14,0x0C, //0010 1010 0110 1110
		0x30,0x34,0x18,0x3C, //0001 1001 0101 1101
		0x24,0x2C,0x1C,0x0A, //0011 1011 0111 1111
	};

	//[type][isCW]
	//the value is 0=end
	//567
	//9 B
	//DEF
	const unsigned short LUT2[16][2]={
		{0x0059,0x007B},{0x00DE,0x0056},{0x00FB,0x00D9},{0x0076,0x00FE},
		{0x05DE,0x057B},{0x0DFB,0x0D56},{0x0F76,0x0FD9},{0x0759,0x07FE},
		{0x59FB,0x7BD9},{0xDE76,0x56FE},{0x57DF,0x57DF},{0x0000,0x0000},
		{0x5DFB,0xD57B},{0xDF76,0xFD56},{0xF759,0x7FD9},{0x75DE,0x57FE},
	};

	if(ed) memset(ed,0,sizeof(TestSolverExtendedData));

	//================ init solver
	if(level.m_nWidth>14 || level.m_nHeight>14){
		printf("[TestSolver] Error: Map too big\n");
		return -2;
	}

	//currently up to 3 players are supported (??? experimental)
	if(level.m_nPlayerCount<1 || level.m_nPlayerCount>3){
		printf("[TestSolver] Error: Invalid player count\n");
		return -2;
	}
	
	int width=level.m_nWidth;
	int height=level.m_nHeight;
	const unsigned char playerXSize=(width<=4)?((width<=2)?1:2):((width<=8)?3:4);
	const unsigned char playerYSize=(height<=4)?((height<=2)?1:2):((height<=8)?3:4);
	unsigned char stateSize=0;
	unsigned char exitPos=0; //position of one of exit (y+1)*16+(x+1)
	TestSolverStateType initState=0;

	//load map
	unsigned char mapData[256];
	memset(mapData,WALL_TILE,sizeof(mapData));
	for(int j=0;j<level.m_nHeight;j++){
		for(int i=0;i<level.m_nWidth;i++){
			unsigned char c=level(i,j);
			switch(c){
			case FLOOR_TILE:
			case WALL_TILE:
			case EMPTY_TILE:
				break;
			case EXIT_TILE:
				exitPos=(j+1)*16+(i+1);
				break;
			case PLAYER_TILE:
				c=0;
				initState|=TestSolverStateType(i|(j<<playerXSize))<<stateSize;
				stateSize+=playerXSize+playerYSize;
				break;
			default:
				printf("[TestSolver] Error: Invalid block type\n");
				return -2;
			}
			mapData[(j+1)*16+(i+1)]=c;
		}
	}

	//load blocks
	TestRotateBlock blocks[TestSolver_MaxBlockCount];
	int blockCount=0;
	for(size_t i=0;i<level.m_objBlocks.size();i++){
		const PushableBlock &obj=*level.m_objBlocks[i];
		if(obj.m_nType!=ROTATE_BLOCK){
			printf("[TestSolver] Error: Invalid block type\n");
			return -2;
		}

		if(blockCount>TestSolver_MaxBlockCount){
			printf("[TestSolver] Error: Too many blocks\n");
			return -2;
		}

		TestRotateBlock block={(obj.m_y+1)*16+(obj.m_x+1),0,0,0};
		unsigned char dir=0;
		for(int j=0;j<4;j++){
			unsigned char c=obj[j];
			if(c>1){
				printf("[TestSolver] Error: Invalid block size\n");
				return -2;
			}
			block.type|=(c<<j);
		}

		block.type=LUT1[block.type&0xF];
		if(block.type==0xFF){
			printf("[TestSolver] Error: Invalid block size\n");
			return -2;
		}

		dir=block.type>>4;
		block.type&=0xF;

		//calculate state bits
		if(block.type!=0xA){
			unsigned char blockStateSize=(block.type==0x8)?1:2;
			block.shift=stateSize;
			stateSize+=blockStateSize;
			if(stateSize>sizeof(TestSolverStateType)*8){
				printf("[TestSolver] Error: Too many blocks\n");
				return -2;
			}
			block.mask=(1<<blockStateSize)-1;
			initState|=((TestSolverStateType)dir)<<block.shift;
		}

		//add block
		mapData[block.pos]=WALL_TILE;
		blocks[blockCount]=block;
		blockCount++;
	}

	//ad-hoc: check additional exit point
	{
		int e=level.m_nExitCount;
		u8string s=toUTF8(level.m_sLevelName);
		u8string::size_type lps=s.find("(E=");
		if(lps!=u8string::npos){
			int x,y;
			if(sscanf(&(s[lps+3]),"%d,%d",&x,&y)==2){
				int p=y*16+x;
				if(x>=1 && x<=width && y>=1 && y<=height && mapData[p]==0){
					exitPos=p;
					mapData[p]=EXIT_TILE;
					e++;
				}
			}
		}
		if(e<=0){
			printf("[TestSolver] Error: Invalid exit count\n");
			return -2;
		}
	}

	//calculate adjacency data
	//bit0-5: index, bit6-7: direction, 0xFF=empty
	unsigned char mapAdjacency[256][4];
	if(!TestSolver_CalculateAdjacancyData(mapAdjacency,mapData,blocks,blockCount)) return -2;

	//TEST: deadlock optimization
	{
		//0=untested, 1=reachable, 2=unreachable (blocked)
		unsigned char reachable[256];
		//0=untested or unreachable (blocked), 1=reachable, 1 bit per direction
		unsigned char blockStateReachable[TestSolver_MaxBlockCount];

		//init
		for(int i=0;i<256;i++){
			unsigned char c;

			if(mapData[i]==WALL_TILE) c=2;
			else{
				int idx=TestSolver_HitTestForBlocks(mapAdjacency,blocks,initState,i);
				c=(idx==-1)?1:((idx>>8)==0xA?2:0);
			}

			reachable[i]=c;
		}
		for(int i=0;i<blockCount;i++){
			unsigned char c=0;
			if(blocks[i].mask){
				c=1<<((unsigned char)(initState>>blocks[i].shift) & blocks[i].mask);
			}
			blockStateReachable[i]=c;
		}

		//iteration
		bool changed;
		do{
			changed=false;

			//check possible state of each block
			for(int i=0;i<blockCount;i++){
				unsigned char m=blocks[i].mask;
				if(m>0){
					unsigned char st=blockStateReachable[i];

					for(unsigned char d=0;d<=m;d++){
						if(st & (1<<d)){
							for(unsigned char ddd=0;ddd<2;ddd++){
								unsigned char d2=(d+1-(ddd<<1))&m;
								if((st & (1<<d2))==0){
									bool b=false;
									//check if player can push it (no double move check)
									for(int dir=0;dir<4;dir++){
										unsigned char pushPos=blocks[i].pos+dirLUT[dir];
										unsigned char playerPos=pushPos+dirLUT[(dir-1+(ddd<<1))&3];
										if(reachable[playerPos]==1 && (HitTestLUT[blocks[i].type+d] & (1<<dir))!=0
											&& mapData[playerPos]!=EMPTY_TILE && mapData[pushPos]!=EMPTY_TILE)
										{
											b=true;
											break;
										}
									}
									//check if block can be rotated
									if(b){
										unsigned int n=LUT2[blocks[i].type+d][ddd];
										do{
											unsigned char checkPos=blocks[i].pos+((n&3)-2)+((n&12)-8)*4;
											if(reachable[checkPos]!=1){
												b=false;
												break;
											}
											n>>=4;
										}while(n&0xF);
										if(b){
											//we can rotate
											st|=(1<<d2);
											//update reachable area
											for(int ii=0;ii<4;ii++){
												unsigned char newPos=blocks[i].pos+dirLUT[ii];
												if((HitTestLUT[blocks[i].type+d] & (1<<ii))!=0 && (HitTestLUT[blocks[i].type+d2] & (1<<ii))==0
													&& reachable[newPos]==0)
												{
													reachable[newPos]=1;
												}
											}
										}
									}
								}
							}
						}
					}

					if(st!=blockStateReachable[i]){
						blockStateReachable[i]=st;
						changed=true;
					}
				}
			}
		}while(changed);

		//now all untested position is unreachable (no code)

		//check rotation block with type 0xA
		for(int i=0;i<blockCount;i++){
			if(!blocks[i].mask){
				bool b=true;
				for(int dir=0;dir<4;dir++){
					unsigned char newPos=blocks[i].pos+dirLUT[dir]+dirLUT[(dir+1)&3];
					if(reachable[newPos]!=1){
						b=false;
						break;
					}
				}
				if(b) blockStateReachable[i]=15;
			}
		}

		//debug output
#if 0
		for(int i=0;i<blockCount;i++){
			printf("Block%d (%d,%d) %d",i,blocks[i].pos&0xF,blocks[i].pos>>4,blockStateReachable[i]);
			if((blockStateReachable[i] & (blockStateReachable[i]-1))==0) printf(" interlocked!");
			printf("\n");
		}
		for(int j=1;j<=height;j++){
			for(int i=1;i<=width;i++){
				printf("%d",reachable[j*16+i]);
			}
			printf("\n");
		}
#endif

		//output extended data
		if(ed){
			memcpy(ed->reachable,reachable,sizeof(reachable));
			memcpy(ed->blockStateReachable,blockStateReachable,sizeof(blockStateReachable));
		}

		//remove deadlocked block
		int deadlockBlockCount=0;
		unsigned char newSize=(playerXSize+playerYSize)*level.m_nPlayerCount;
		TestSolverStateType newState=initState&((TestSolverStateType(1)<<newSize)-1);

		for(int i=0;i<blockCount;i++){
			if((blockStateReachable[i] & (blockStateReachable[i]-1))==0){
				//blocked
				unsigned char ht=HitTestLUT[blocks[i].type+((initState>>blocks[i].shift)&blocks[i].mask)];
				for(int ii=0;ii<4;ii++){
					if(ht & (1<<ii)) mapData[blocks[i].pos+dirLUT[ii]]=WALL_TILE;
				}
				deadlockBlockCount++;
			}else{
				int ii=i-deadlockBlockCount;
				if(blocks[i].mask){
					newState|=((TestSolverStateType)((initState>>blocks[i].shift)&blocks[i].mask))<<newSize;
					blocks[i].shift=newSize;
					newSize+=(blocks[i].type==0x8)?1:2;
				}
				if(deadlockBlockCount>0) blocks[ii]=blocks[i];
			}
		}

		if(ed) ed->deadlockBlockCount=deadlockBlockCount;

		//recalculate adjacency data
		if(deadlockBlockCount>0){
#ifdef _DEBUG
			printf("[TestSolver] Debug: Optimized out %d rotate block(s)\n",deadlockBlockCount);
#endif
			initState=newState;
			stateSize=newSize;
			blockCount-=deadlockBlockCount;
			if(!TestSolver_CalculateAdjacancyData(mapAdjacency,mapData,blocks,blockCount)) return -2;
		}
	}

	//debug
#ifdef  _DEBUG
	printf("[TestSolver] Debug: StateSize=%d (XSize=%d,YSize=%d), InitState="
#ifdef FORCE_64BIT_SOLVER_STATE
#ifdef WIN32
		"%016I64X"
#else
		"%016LX"
#endif
#else
		"%p"
#endif
		"\n",stateSize,playerXSize,playerYSize,
#ifdef FORCE_64BIT_SOLVER_STATE
		initState
#else
		(void*)initState
#endif
		);
#endif

	//================ run solver
	//use hashmap (??? experimental) no, it runs slower
	//TODO: don't use vector at all
	//SimpleHashMap<10,TestSolverStateType,int> nodeMap;
	std::map<TestSolverStateType,int> nodeMap;
	std::vector<TestSolverNode> nodes;
	size_t currentIndex=0;

	rec.clear();

	const unsigned char playerXMask=(1<<playerXSize)-1;
	const unsigned char playerYMask=(1<<playerYSize)-1;

	const unsigned int exitState=((exitPos&0xF)-1)|((((exitPos>>4)&0xF)-1)<<playerXSize);

	//create initial node
	{
		int playerRemaining=level.m_nPlayerCount;

		unsigned char shift=0;
		for(int i=0;i<level.m_nPlayerCount;i++){
			unsigned char x=(initState>>shift)&playerXMask,y=(initState>>(shift+playerXSize))&playerYMask;
			unsigned char pos=(y+1)*16+(x+1);
			if(mapData[pos]==EXIT_TILE){
				initState=(initState&~(((TestSolverStateType(1)<<(playerXSize+playerYSize))-1)<<shift))
					|(TestSolverStateType(exitState)<<shift);
				playerRemaining--;
			}
			shift+=playerXSize+playerYSize;
		}

		if(playerRemaining==0) return 1;

		initState=TestSolver_SortPlayerPosition(level.m_nPlayerCount,playerXSize+playerYSize,initState);

		TestSolverNode node={-1,initState};
		nodes.push_back(node);
		nodeMap[initState]=0;
	}

	do{
		TestSolverNode node=nodes[currentIndex];

		int playerRemaining=level.m_nPlayerCount;

		unsigned char pos[4];

		unsigned char shift=0;
		for(int i=0;i<level.m_nPlayerCount;i++){
			unsigned char x=(node.state>>shift)&playerXMask;
			unsigned char y=(node.state>>(shift+playerXSize))&playerYMask;
			if((pos[i]=(y+1)*16+(x+1))==exitPos) playerRemaining--;
			shift+=playerXSize+playerYSize;

			//adhoc????
			if(mapData[pos[i]]==FLOOR_TILE) mapData[pos[i]]=WALL_TILE;
		}

		//for each player
		shift=0;
		for(int currentPlayer=0;currentPlayer<level.m_nPlayerCount;currentPlayer++){
			if(pos[currentPlayer]==exitPos) continue;

			//adhoc???
			if(mapData[pos[currentPlayer]]==WALL_TILE) mapData[pos[currentPlayer]]=FLOOR_TILE;

			//for each direction
			for(int d=0;d<4;d++){
				char dir=dirLUT[d];
				unsigned char newPos=pos[currentPlayer]+dir;
				unsigned char c=mapData[newPos];

				if(c==FLOOR_TILE || c==EXIT_TILE){
					//it's movable, now check for blocks
					int blockIdx=TestSolver_HitTestForBlocks(mapAdjacency,blocks,node.state,newPos);
					TestSolverStateType newState=node.state & ~(((((TestSolverStateType)1)<<(playerXSize+playerYSize))-1)<<shift);

					if(blockIdx!=-1){
						TestRotateBlock block=blocks[blockIdx&0x3F];
						int tmp=d-((blockIdx>>6)&3);
						if(tmp&1){
							//check if block can be rotated
							unsigned int n=LUT2[(blockIdx>>8)&0xF][(tmp&2)>>1];
							bool b=true;

							do{
								unsigned char checkPos=block.pos+((n&3)-2)+((n&12)-8)*4;

								if(mapData[checkPos]==WALL_TILE || TestSolver_HitTestForBlocks(mapAdjacency,blocks,node.state,checkPos)!=-1){
									b=false;
									break;
								}

								n>>=4;
							}while(n&0xF);

							if(b){
								//can be rotated, update new state
								unsigned char newDir=0;
								if(block.mask){
									newDir=((node.state>>block.shift)+(1-(tmp&2)))&block.mask;
									newState&=~(((TestSolverStateType)block.mask)<<block.shift);
									newState|=((TestSolverStateType)newDir)<<block.shift;
								}

								//check if we should move twice
								if(HitTestLUT[block.type+newDir] & (1<<((blockIdx>>6)&3))){
									newPos+=dir;
									if((c=mapData[newPos])!=EMPTY_TILE) blockIdx=-1;
								}else{
									blockIdx=-1;
								}
							}
						}
					}

					if(blockIdx==-1){
						if(c==EXIT_TILE && playerRemaining==1){
							//get player position of each step
							struct PlayerPosition{
								unsigned char& operator[](int index){
									return p[index];
								}
								unsigned char p[4];
							};

							std::vector<PlayerPosition> positions;
							PlayerPosition newPos={exitPos,exitPos,exitPos,exitPos};
							positions.push_back(newPos);

							int idx=currentIndex;
							do{
								TestSolverNode node=nodes[idx];

								PlayerPosition pos;
								unsigned char shift=0;
								for(int i=0;i<level.m_nPlayerCount;i++){
									unsigned char x=(node.state>>shift)&playerXMask,y=(node.state>>(shift+playerXSize))&playerYMask;
									pos[i]=y*16+x;
#ifdef USE_SOLUTION_REACHABLE
									if(ed && pos[i]!=exitPos /* ??? */) ed->solutionReachable[pos[i]]=1;
#endif
									pos[i]+=17;

									shift+=playerXSize+playerYSize;
								}

								//determine which player is moved
								int index=-1,newIndex=-1,newIndexMask=(1<<level.m_nPlayerCount)-1;
								for(int i=0;i<level.m_nPlayerCount;i++){
									bool b=true;
									for(int j=0;j<level.m_nPlayerCount;j++){
										if(pos[i]==newPos[j] && (newIndexMask&(1<<j))!=0){
											newIndexMask&=~(1<<j);
											b=false;
											break;
										}
									}
									if(b){
										assert(index<0);
										index=i;
									}
								}
								assert(newIndexMask && (newIndexMask&(newIndexMask-1))==0);
								switch(newIndexMask){
								case 1: newIndex=0; break;
								case 2: newIndex=1; break;
								case 4: newIndex=2; break; //no more than 3 players
								}

								//update position
								newPos[newIndex]=pos[index];
								positions.push_back(newPos);

								idx=node.parent;
							}while(idx>=0);

							//generate solution
							int prevPlayer=-1;
							for(int i=positions.size()-1;i>0;i--){
								int currentPlayer=-1;

								for(int j=0;j<level.m_nPlayerCount;j++){
									if(positions[i][j]!=positions[i-1][j]){
										currentPlayer=j;
										break;
									}
								}
								assert(currentPlayer>=0);

								unsigned char pos=positions[i][currentPlayer],
									newPos=positions[i-1][currentPlayer];

								//check if go to exit
								if(newPos==exitPos){
									for(int i=0;i<4;i++){
										char d=dirLUT[i];
										if(mapData[pos+d]==EXIT_TILE){
											newPos=pos+d;
											break;
										}
									}
								}

								if(currentPlayer!=prevPlayer){
									if(level.m_nPlayerCount>1){
										//we need to switch player
										rec.append(str(MyFormat("(%d,%d)")<<(pos&0xF)<<((pos>>4)&0xF)));
									}
									prevPlayer=currentPlayer;
								}

								//generate one step
								rec.push_back((newPos>pos)?(newPos>pos+2?'S':'D'):(newPos<pos-2?'W':'A'));
							}

							//generate extended data
							if(ed){
								char blockUsed[TestSolver_MaxBlockCount]={};

								int idx=currentIndex;
								TestSolverStateType st=newState;
								ed->pushes=0;

								unsigned char shift=(playerXSize+playerYSize)*level.m_nPlayerCount;

								do{
									TestSolverNode node=nodes[idx];

									//check if some block is moved
									st^=node.state;
									if(st>>shift){
										ed->pushes++;
										for(int i=0;i<blockCount;i++){
											if(((unsigned char)(st>>blocks[i].shift)) & blocks[i].mask){
												blockUsed[i]=1;
												break;
											}
										}
									}

									idx=node.parent;
									st=node.state;
								}while(idx>=0);

								idx=0;
								for(int i=0;i<blockCount;i++){
									if(blockUsed[i]) idx++;
								}

								ed->state.nAllNodeCount=nodes.size();
								ed->state.nOpenedNodeCount=currentIndex;
								ed->blockUsed=idx;
							}

							//debug
#ifdef _DEBUG
							printf("[TestSolver] Debug: Solution found. Nodes=%d (Opened=%d), Step=%d\n",nodes.size(),currentIndex,rec.size());
#endif

							//DEBGU
							printf("Time=%dms\n",SDL_GetTicks()-ttt);

							return 1;
						}

						//add node
						if(mapData[newPos]==EXIT_TILE) newPos=exitPos;
						newPos-=17;
						newState=TestSolver_SortPlayerPosition(level.m_nPlayerCount,
							playerXSize+playerYSize,
							newState|TestSolverStateType((newPos&playerXMask)|(((newPos>>4)&playerYMask)<<playerXSize))<<shift);
						if(nodeMap.find(newState)==nodeMap.end()){
							TestSolverNode newNode={currentIndex,newState};
							nodes.push_back(newNode);
							nodeMap[newState]=nodes.size()-1;
						}
					}
				}
			}

			//adhoc???
			if(mapData[pos[currentPlayer]]==FLOOR_TILE) mapData[pos[currentPlayer]]=WALL_TILE;

			shift+=playerXSize+playerYSize;
		}

		//ad-hoc???
		for(int i=0;i<level.m_nPlayerCount;i++){
			if(mapData[pos[i]]==WALL_TILE) mapData[pos[i]]=FLOOR_TILE;
		}

		//next node
		if(((++currentIndex) & 0xFFFF)==0 && callback){
			LevelSolverState progress={
				nodes.size(),
				currentIndex,
			};
			if(callback(userData,progress)) return -1;
		}
	}while(currentIndex<nodes.size());

	//generate extended data
	if(ed){
		ed->state.nAllNodeCount=ed->state.nOpenedNodeCount=currentIndex;
		//TODO: number of blocks reachable
	}

	//debug
#ifdef _DEBUG
	printf("[TestSolver] Debug: Solution not found. Nodes=%d\n",nodes.size());
#endif

	return 0;
}

int RunSolver(SolverType type,const PuzzleBoyLevel& level,u8string& rec,void* userData,LevelSolverCallback callback){
	switch(type){
	case TEST_SOLVER: return TestSolver_SolveIt(level,rec,userData,callback);
	default: return -2;
	}
}
