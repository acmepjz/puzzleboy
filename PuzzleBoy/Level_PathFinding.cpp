#include "PuzzleBoyLevel.h"
#include "PushableBlock.h"
#include "RecordManager.h"

#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <vector>
#include <map>
#include <queue>

#define PATH_FINDING_MOVEABLE 0
#define PATH_FINDING_BLOCKED 1
#define PATH_FINDING_HORIZONTAL_SKIP 2
#define PATH_FINDING_VERTICAL_SKIP 3

bool PuzzleBoyLevel::FindPath(u8string& ret,int x1,int y1,int x0,int y0) const{
	if(x0<0 || y0<0){
		x0=m_nPlayerX;
		y0=m_nPlayerY;
	}
	if(x0==x1 && y0==y1){
		ret.clear();
		return true;
	}
	if(x0<0 || x0>=m_nWidth || y0<0 || y0>=m_nHeight) return false;
	if(x1<0 || x1>=m_nWidth || y1<0 || y1>=m_nHeight) return false;

	//create moveable data
	std::vector<unsigned char> bMoveableData; //0=moveable, 1=blocked, 2=horizontal skip, 3=vertical skip
	bMoveableData.resize(m_nWidth*m_nHeight);

	for(int i=0;i<(int)m_bMapData.size();i++){
		switch(m_bMapData[i]){
		case 0:
			bMoveableData[i]=0;
			break;
		case EXIT_TILE:
			bMoveableData[i]=(m_nTargetFinished==m_nTargetCount)?0:1; //???
			break;
		default:
			bMoveableData[i]=1;
			break;
		}
	}

	for(int i=0;i<(int)m_objBlocks.size();i++){
		PushableBlock *objBlock=m_objBlocks[i];
		switch(objBlock->m_nType){
		case ROTATE_BLOCK:
			{
				//check if four arms are of equal length
				bool bHitTest=false;
				int m=(*objBlock)[0];
				if(m==(*objBlock)[1] && m==(*objBlock)[2] && m==(*objBlock)[3]){
					//check if there are no obstacles
					int m2=m*(m+2); // (r+1)^2-1

					for(int yy=-m;yy<=m;yy++){
						for(int xx=-m;xx<=m;xx++){
							if(xx*xx+yy*yy<=m2){
								int xxx=xx+objBlock->m_x;
								int yyy=yy+objBlock->m_y;
								if((xxx!=x0 || yyy!=y0) && HitTestForPlacingBlocks(xxx,yyy,i)!=-1){
									bHitTest=true;
									break;
								}
							}
						}
						if(bHitTest) break;
					}
				}else{
					bHitTest=true;
				}

				if(objBlock->m_x>=0 && objBlock->m_x<m_nWidth){
					int xx=objBlock->m_x;
					for(int y=-int((*objBlock)[0]);y<=int((*objBlock)[2]);y++){
						int yy=y+objBlock->m_y;
						if(yy<0 || yy>=m_nHeight) continue;

						int idx=yy*m_nWidth+xx;
						if(bMoveableData[idx]==0) bMoveableData[idx]=(bHitTest || y==0)?1:PATH_FINDING_HORIZONTAL_SKIP;
					}
				}
				if(objBlock->m_y>=0 && objBlock->m_y<m_nHeight){
					int yy=objBlock->m_y;
					for(int x=-int((*objBlock)[1]);x<=int((*objBlock)[3]);x++){
						int xx=x+objBlock->m_x;
						if(xx<0 || xx>=m_nWidth) continue;

						int idx=yy*m_nWidth+xx;
						if(bMoveableData[idx]==0) bMoveableData[idx]=(bHitTest || x==0)?1:PATH_FINDING_VERTICAL_SKIP;
					}
				}
			}
			break;
		default:
			for(int y=0;y<objBlock->m_h;y++){
				int yy=y+objBlock->m_y;
				if(yy<0 || yy>=m_nHeight) continue;

				for(int x=0;x<objBlock->m_w;x++){
					int xx=x+objBlock->m_x;
					if(xx<0 || xx>=m_nWidth) continue;

					if((*objBlock)(x,y)) bMoveableData[yy*m_nWidth+xx]=1;
				}
			}
			break;
		}
	}

	//check moveable data
	if(bMoveableData[y1*m_nWidth+x1]) return false;

	//start BFS :-3
	std::vector<int> nodes;
	nodes.resize(m_nWidth*m_nHeight);
	memset(&(nodes[0]),-1,sizeof(int)*m_nWidth*m_nHeight);

	std::queue<int> queuedNodes;

	int currentNode=(y0<<16)|x0;
	int endNode=(y1<<16)|x1;
	nodes[y0*m_nWidth+x0]=currentNode;
	queuedNodes.push(currentNode);

	while(!queuedNodes.empty()){
		currentNode=queuedNodes.front();
		queuedNodes.pop();

		if(currentNode==endNode){
			//found the path
			ret.clear();

			for(;;){
				//get previous node
				endNode=nodes[y1*m_nWidth+x1];

				//get previous coordinate
				x0=endNode & 0xFFFF;
				y0=(endNode>>16) & 0xFFFF;
				if(x0==x1 && y0==y1) break;

				//get direction
				ret.push_back(x1<x0?'A':y1<y0?'W':x1>x0?'D':'S');

				//advance
				x1=x0;
				y1=y0;
			}

			//string reverse
			int m=ret.size();
			for(int i=0;i<m/2;i++){
				char ch=ret[i];
				ret[i]=ret[m-1-i];
				ret[m-1-i]=ch;
			}

			//over
			return true;
		}

		//expand node
		x0=currentNode & 0xFFFF;
		y0=(currentNode>>16) & 0xFFFF;

#define EXPAND_1(DX,DY,SKIP_FLAGS) \
	int xx=x0+DX,yy=y0+DY; \
	int idx=yy*m_nWidth+xx; \
	switch(bMoveableData[idx]){ \
	case SKIP_FLAGS:

#define EXPAND_2() \
		if(bMoveableData[idx]) break; \
		/* fall through */ \
	case 0: \
		if(nodes[idx]==-1){ \
			nodes[idx]=currentNode; \
			queuedNodes.push((yy<<16)|xx); \
		} \
		break; \
	default: \
		break; \
	}

		//left
		if(x0>0){
			EXPAND_1(-1,0,PATH_FINDING_HORIZONTAL_SKIP);
			xx--; idx--; assert(xx>=0);
			EXPAND_2();
		}

		//right
		if(x0<m_nWidth-1){
			EXPAND_1(1,0,PATH_FINDING_HORIZONTAL_SKIP);
			xx++; idx++; assert(xx<m_nWidth);
			EXPAND_2();
		}

		//up
		if(y0>0){
			EXPAND_1(0,-1,PATH_FINDING_VERTICAL_SKIP);
			yy--; idx-=m_nWidth; assert(yy>=0);
			EXPAND_2();
		}

		//down
		if(y0<m_nHeight-1){
			EXPAND_1(0,1,PATH_FINDING_VERTICAL_SKIP);
			yy++; idx+=m_nWidth; assert(yy<m_nHeight);
			EXPAND_2();
		}
	}

	return false;
}

#undef EXPAND_1
#undef EXPAND_2

struct BlockBFSNodeRef{
	int index;
	int nEstimatedStep; //nBestStep+nEstimatedDistance

	bool operator<(const BlockBFSNodeRef& other) const{
		return nEstimatedStep>other.nEstimatedStep;
	}
};

struct BlockBFSNode{
	int x,y;
	int pidx; //player pos index. -1=current player pos, otherwise read from array
	int nBestStep;
	//int nEstimatedStep; //nBestStep+nEstimatedDistance
	int idxPrev;

	//a stupid one
	int Hash() const{
		return pidx^(x<<8)^(y<<16);
	}
};

inline int L1Norm(int x0,int y0,int x1,int y1){
	x0-=x1;
	y0-=y1;
	return (x0<0?-x0:x0)+(y0<0?-y0:y0);
}

#define PATH_FINDING_BLOCKED_MASK 15
#define PATH_FINDING_EMPTY 16

bool PuzzleBoyLevel::FindPathForBlock(u8string& ret,int index,int x1,int y1) const{
	if(index<0 || index>=(int)m_objBlocks.size()) return false;

	const PushableBlock *targetBlock=m_objBlocks[index];

	switch(targetBlock->m_nType){
	case NORMAL_BLOCK:
	case TARGET_BLOCK:
		break;
	default:
		return false;
	}

	//block unmoved?
	if(targetBlock->m_x==x1 && targetBlock->m_y==y1){
		ret.clear();
		return true;
	}

	//--------create moveable data
	//0=moveable, 1=blocked
	//2=horizontal skip, 3=vertical skip (with rotate block index x>>8)
	//16=empty (box but not player)
	std::vector<unsigned int> bMoveableData;
	bMoveableData.resize(m_nWidth*m_nHeight);

	{
		int pp=m_nPlayerY*m_nWidth+m_nPlayerX;

		for(int i=0;i<(int)m_bMapData.size();i++){
			switch(m_bMapData[i]){
			case 0:
				bMoveableData[i]=0;
				break;
			case EMPTY_TILE:
				bMoveableData[i]=PATH_FINDING_EMPTY;
				break;
			case PLAYER_TILE:
				bMoveableData[i]=(i==pp)?0:1;
				break;
			default:
				bMoveableData[i]=1;
				break;
			}
		}
	}

	for(int i=0;i<(int)m_objBlocks.size();i++){
		if(i==index) continue;

		const PushableBlock *objBlock=m_objBlocks[i];
		switch(objBlock->m_nType){
		case ROTATE_BLOCK:
			{
				//check if four arms are of equal length
				bool bHitTest=false;
				int m=(*objBlock)[0];
				if(m==(*objBlock)[1] && m==(*objBlock)[2] && m==(*objBlock)[3]){
					//check if there are no obstacles
					int m2=m*(m+2); // (r+1)^2-1

					for(int yy=-m;yy<=m;yy++){
						for(int xx=-m;xx<=m;xx++){
							if(xx*xx+yy*yy<=m2){
								int xxx=xx+objBlock->m_x;
								int yyy=yy+objBlock->m_y;
								if(xxx==m_nPlayerX && yyy==m_nPlayerY) continue;

								if(HitTestForPlacingBlocks(xxx,yyy,(xx==0 || yy==0)?i:index)!=-1){
									bHitTest=true;
									break;
								}
							}
						}
						if(bHitTest) break;
					}
				}else{
					bHitTest=true;
				}

				if(objBlock->m_x>=0 && objBlock->m_x<m_nWidth){
					int xx=objBlock->m_x;
					for(int y=-int((*objBlock)[0]);y<=int((*objBlock)[2]);y++){
						int yy=y+objBlock->m_y;
						if(yy<0 || yy>=m_nHeight) continue;

						int idx=yy*m_nWidth+xx;
						if(bMoveableData[idx]) bMoveableData[idx]=1;
						else bMoveableData[idx]=(bHitTest || y==0)?1:(PATH_FINDING_HORIZONTAL_SKIP|(i<<8));
					}
				}
				if(objBlock->m_y>=0 && objBlock->m_y<m_nHeight){
					int yy=objBlock->m_y;
					for(int x=-int((*objBlock)[1]);x<=int((*objBlock)[3]);x++){
						int xx=x+objBlock->m_x;
						if(xx<0 || xx>=m_nWidth) continue;

						int idx=yy*m_nWidth+xx;
						if(bMoveableData[idx]) bMoveableData[idx]=1;
						else bMoveableData[idx]=(bHitTest || x==0)?1:(PATH_FINDING_VERTICAL_SKIP|(i<<8));
					}
				}
			}
			break;
		default:
			for(int y=0;y<objBlock->m_h;y++){
				int yy=y+objBlock->m_y;
				if(yy<0 || yy>=m_nHeight) continue;

				for(int x=0;x<objBlock->m_w;x++){
					int xx=x+objBlock->m_x;
					if(xx<0 || xx>=m_nWidth) continue;

					if((*objBlock)(x,y)) bMoveableData[yy*m_nWidth+xx]=1;
				}
			}
			break;
		}
	}

	//check moveable data
	for(int j=0;j<targetBlock->m_h;j++){
		for(int i=0;i<targetBlock->m_w;i++){
			if((*targetBlock)(i,j)){
				int x=i+x1;
				int y=j+y1;
				if(x<0 || y<0 || x>=m_nWidth || y>=m_nHeight
					|| (bMoveableData[y*m_nWidth+x] & PATH_FINDING_BLOCKED_MASK)!=0) return false;
			}
		}
	}

	//check possible adjacent position
	std::vector<int> possibleAdjacentX;
	std::vector<int> possibleAdjacentY;
	std::vector<u8string> tempSolution;
	{
		std::vector<unsigned char> md;
		std::vector<int> theStack;
		int ww=targetBlock->m_w+2;
		int hh=targetBlock->m_h+2;

		md.resize(ww*hh);
		memset(&(md[0]),0,ww*hh);

		int i=m_nPlayerX-targetBlock->m_x+1;
		int j=m_nPlayerY-targetBlock->m_y+1;
		if(i<0) i=0; else if(i>=ww) i=ww-1;
		if(j<0) j=0; else if(j>=hh) j=hh-1;

		//blocked in the block?
		if(i>0 && i<ww-1 && j>0 && j<hh-1 && (*targetBlock)(i-1,j-1)) return false;

		//run DFS (flood fill)
		md[j*ww+i]=1;
		theStack.push_back((j<<16)|i);
		while(!theStack.empty()){
			i=theStack[theStack.size()-1];
			theStack.pop_back();

			j=i>>16;
			i&=0x0000FFFF;

#define EXPAND_1(CONDITION,DX,DY) if(CONDITION){ \
		int xx=i+DX,yy=j+DY; \
		int idx=yy*ww+xx; \
		if(md[idx]==0 && (xx<=0 || xx>=ww-1 || yy<=0 || yy>=hh-1 || (*targetBlock)(xx-1,yy-1)==0)){ \
			md[idx]=1; \
			theStack.push_back((yy<<16)|xx); \
		} \
	}

			//left
			EXPAND_1(i>0,-1,0);

			//right
			EXPAND_1(i<ww-1,1,0);

			//up
			EXPAND_1(j>0,0,-1);

			//down
			EXPAND_1(j<hh-1,0,1);

#undef EXPAND_1
		}

		//check boundary
		for(j=0;j<hh;j++){
			for(i=0;i<ww;i++){
				int idx=j*ww+i;

				if(md[idx]!=0 && (
					(i>0 && md[idx-1]==0) ||
					(i<ww-1 && md[idx+1]==0) ||
					(j>0 && md[idx-ww]==0) ||
					(j<hh-1 && md[idx+ww]==0)
					))
				{
					possibleAdjacentX.push_back(i-1);
					possibleAdjacentY.push_back(j-1);
				}
			}
		}
	}

	if(possibleAdjacentX.empty()) return false;
	tempSolution.resize(possibleAdjacentX.size());

	std::vector<BlockBFSNodeRef> unopenedNodes; //a heap

	std::vector<BlockBFSNode> checkedNodes;
	std::multimap<int,int> checkedMap;
	std::vector<u8string> solutionOfNodes; //(reversed) solution from previous node

	std::vector<unsigned int> bTempData;
	bTempData.resize(m_nWidth*m_nHeight);
	memset(&(bTempData[0]),0,m_nWidth*m_nHeight*sizeof(unsigned int));
	unsigned int nTimeStamp=1;
	const unsigned int nTimeStampDelta=8;

	//create init nodes
	{
		BlockBFSNode root={targetBlock->m_x,targetBlock->m_y,-1};
		FindPath2(targetBlock,root,&(possibleAdjacentX[0]),&(possibleAdjacentY[0]),possibleAdjacentX.size(),
			&(bMoveableData[0]),&(bTempData[0]),nTimeStamp,&(tempSolution[0]));
		nTimeStamp+=nTimeStampDelta;

		BlockBFSNode node={targetBlock->m_x,targetBlock->m_y,0,0,-1};
		int estimatedDist=L1Norm(targetBlock->m_x,targetBlock->m_y,x1,y1);

		for(int i=0;i<(int)possibleAdjacentX.size();i++){
			//no solution?
			int len=tempSolution[i].size();
			if(len==1 && tempSolution[i][0]=='?') continue;

			//create node
			node.pidx=i;
			node.nBestStep=len;

			//add node
			checkedNodes.push_back(node);
			checkedMap.insert(std::pair<int,int>(node.Hash(),checkedNodes.size()-1));
			solutionOfNodes.push_back(tempSolution[i]);

			BlockBFSNodeRef ref={checkedNodes.size()-1,len+estimatedDist};
			unopenedNodes.push_back(ref);
		}
	}

	if(unopenedNodes.empty()) return false;

	std::make_heap(unopenedNodes.begin(),unopenedNodes.end());

	//start BFS
	int nCurrentBestStep=0x7FFFFFFF;
	const int nDirections[4][4]={
		{-1,0,'A',0},
		{1,0,'D',0},
		{0,-1,'W',0},
		{0,1,'S',0},
	};

	while(!unopenedNodes.empty()){
		//get current best
		BlockBFSNodeRef ref=unopenedNodes.front();

		//check if it's impossible to get better result
		if(ref.nEstimatedStep>nCurrentBestStep) break;

		//remove it from heap
		std::pop_heap(unopenedNodes.begin(),unopenedNodes.end());
		unopenedNodes.pop_back();

		BlockBFSNode node=checkedNodes[ref.index];
		int x0=possibleAdjacentX[node.pidx];
		int y0=possibleAdjacentY[node.pidx];

		//for each direction
		for(int dir=0;dir<4;dir++){
			int dx=nDirections[dir][0];
			int dy=nDirections[dir][1];

			//get position relative to the block
			int xx=x0+dx;
			int yy=y0+dy;

			//check if it's pushable
			if(xx<0 || xx>=targetBlock->m_w || yy<0 || yy>=targetBlock->m_h
				|| (*targetBlock)(xx,yy)==0) continue;

			//not it's absolute position
			xx+=node.x;
			yy+=node.y;

			//it should be true unless the block is at invalid position
			assert(xx>=0 && xx<m_nWidth && yy>=0 && yy<m_nHeight);

			//check if the player is not blocked
			if(bMoveableData[yy*m_nWidth+xx]) continue;

			//check if the block is not blocked
			bool bHitTest=false;
			for(int j=0;j<targetBlock->m_h;j++){
				for(int i=0;i<targetBlock->m_w;i++){
					if((*targetBlock)(i,j)){
						int xxx=node.x+dx+i;
						int yyy=node.y+dy+j;
						if(xxx<0 || xxx>=m_nWidth || yyy<0 || yyy>=m_nHeight
							|| (bMoveableData[yyy*m_nWidth+xxx] & PATH_FINDING_BLOCKED_MASK)!=0)
						{
							bHitTest=true;
							break;
						}
					}
				}
				if(bHitTest) break;
			}
			if(bHitTest) continue;

			//we can move it
			BlockBFSNode newNode=node;
			newNode.x+=dx;
			newNode.y+=dy;
			newNode.idxPrev=ref.index;
			int estimatedDist=L1Norm(newNode.x,newNode.y,x1,y1);

			//try to move the player to all adjacent positions of the block
			FindPath2(targetBlock,newNode,&(possibleAdjacentX[0]),&(possibleAdjacentY[0]),possibleAdjacentX.size(),
				&(bMoveableData[0]),&(bTempData[0]),nTimeStamp,&(tempSolution[0]));
			nTimeStamp+=nTimeStampDelta;

			for(int i=0;i<(int)possibleAdjacentX.size();i++){
				//no solution?
				int len=tempSolution[i].size();
				if(len==1 && tempSolution[i][0]=='?') continue;

				//update node
				newNode.pidx=i;
				newNode.nBestStep=node.nBestStep+1+len;

				//check if it's impossible to get better result
				if(newNode.nBestStep+estimatedDist>nCurrentBestStep) continue;

				//check if it's visited
				int hash=newNode.Hash();
				int idxOfNewNode=-1;
				for(std::pair<std::multimap<int,int>::const_iterator,std::multimap<int,int>::const_iterator>
					it=checkedMap.equal_range(hash);it.first!=it.second;++it.first)
				{
					int ii=it.first->second;
					if(checkedNodes[ii].x==newNode.x && checkedNodes[ii].y==newNode.y
						&& checkedNodes[ii].pidx==newNode.pidx)
					{
						idxOfNewNode=ii;
						break;
					}
				}

				if(idxOfNewNode<0){
					//it's unvisited, add node
					checkedNodes.push_back(newNode);
					checkedMap.insert(std::pair<int,int>(hash,checkedNodes.size()-1));
					tempSolution[i].push_back(nDirections[dir][2]);
					solutionOfNodes.push_back(tempSolution[i]);

					BlockBFSNodeRef ref={
						checkedNodes.size()-1,
						newNode.nBestStep+estimatedDist
					};
					unopenedNodes.push_back(ref);
					std::push_heap(unopenedNodes.begin(),unopenedNodes.end());
				}else if(newNode.nBestStep<checkedNodes[idxOfNewNode].nBestStep){
					//we get a better result of existing node
					checkedNodes[idxOfNewNode]=newNode;
					tempSolution[i].push_back(nDirections[dir][2]);
					solutionOfNodes[idxOfNewNode]=tempSolution[i];

					//FIXME: expand again?? may cause dead loops
					BlockBFSNodeRef ref={
						idxOfNewNode,
						newNode.nBestStep+estimatedDist
					};
					unopenedNodes.push_back(ref);
					std::push_heap(unopenedNodes.begin(),unopenedNodes.end());
				}

				//check if we get a better solution
				if(estimatedDist==0 && newNode.nBestStep<nCurrentBestStep){
					nCurrentBestStep=newNode.nBestStep;
				}
			}
		}
	}

	//check if goal node is visited
	nCurrentBestStep=0x7FFFFFFF;
	int nCurrentBestIndex=-1;
	for(int i=0;i<(int)possibleAdjacentX.size();i++){
		BlockBFSNode node={x1,y1,i};
		int hash=node.Hash();
		for(std::pair<std::multimap<int,int>::const_iterator,std::multimap<int,int>::const_iterator>
			it=checkedMap.equal_range(hash);it.first!=it.second;++it.first)
		{
			int ii=it.first->second;
			if(checkedNodes[ii].x==x1 && checkedNodes[ii].y==y1
				&& checkedNodes[ii].nBestStep<nCurrentBestStep)
			{
				nCurrentBestStep=checkedNodes[ii].nBestStep;
				nCurrentBestIndex=ii;
			}
		}
	}

	if(nCurrentBestIndex<0) return false;

	//generate the solution
	ret.clear();
	while(nCurrentBestIndex>=0){
		ret.append(solutionOfNodes[nCurrentBestIndex]);
		nCurrentBestIndex=checkedNodes[nCurrentBestIndex].idxPrev;
	}

	//a trivial sanity check
	assert(ret.size()==nCurrentBestStep);

	//string reverse
	for(int i=0,m=ret.size();i<m/2;i++){
		char ch=ret[i];
		ret[i]=ret[m-1-i];
		ret[m-1-i]=ch;
	}

	//over
	return true;
}

#define PATH_TIMESTAMP_BLOCKED 0
#define PATH_TIMESTAMP_NOT_BLOCKED 1
#define PATH_TIMESTAMP_LEFT 2
#define PATH_TIMESTAMP_RIGHT 3
#define PATH_TIMESTAMP_UP 4
#define PATH_TIMESTAMP_DOWN 5
#define PATH_TIMESTAMP_TARGET 6

void PuzzleBoyLevel::FindPath2(const PushableBlock *targetBlock,const BlockBFSNode& src,const int* possibleAdjacentX,const int* possibleAdjacentY,int possibleAdjacentCount,const unsigned int* bMoveableData,unsigned int* bTempData,unsigned int nTimeStamp,u8string* ret) const{
	//get current pos
	int x00,y00;
	if(src.pidx<0){
		x00=m_nPlayerX;
		y00=m_nPlayerY;
	}else{
		x00=src.x+possibleAdjacentX[src.pidx];
		y00=src.y+possibleAdjacentY[src.pidx];
	}

	//update temp moveable data
	//relative to timestamp: <0: moveable/unconfirmed
	//0=blocked (e.g. by current moveable block or blocked horizontal/vertical skip)
	//1=confirmed usable horizontal/vertical skip
	//2,3,4,5=visited node with direction left,right,up,down
	//6=target

	for(int j=0;j<targetBlock->m_h;j++){
		for(int i=0;i<targetBlock->m_w;i++){
			if((*targetBlock)(i,j)){
				int x=src.x+i;
				int y=src.y+j;
				if(x>=0 && x<m_nWidth && y>=0 && y<m_nHeight){
					bTempData[y*m_nWidth+x]=nTimeStamp;
				}
			}
		}
	}

	//check if destination is blocked
	int nDestCount=0;
	for(int i=0;i<possibleAdjacentCount;i++){
		int x=src.x+possibleAdjacentX[i];
		int y=src.y+possibleAdjacentY[i];
		int idx=y*m_nWidth+x;

		if(x>=0 && x<m_nWidth && y>=0 && y<m_nHeight
			&& bMoveableData[idx]==0 && bTempData[idx]<nTimeStamp)
		{
			if(x==x00 && y==y00){
				bTempData[idx]=nTimeStamp+PATH_TIMESTAMP_LEFT; //FIXME: ad-hoc trick to mark visited
				ret[i].clear();
			}else{
				bTempData[idx]=nTimeStamp+PATH_TIMESTAMP_TARGET;
				nDestCount++;
				ret[i]="?";
			}
		}else{
			ret[i]="?";
		}
	}
	if(nDestCount==0) return;

	//start BFS
	std::queue<int> queuedNodes;

	int currentNode=(y00<<16)|x00;
	queuedNodes.push(currentNode);

	while(!queuedNodes.empty() && nDestCount>0){
		currentNode=queuedNodes.front();
		queuedNodes.pop();

		//expand node
		int x0=currentNode & 0xFFFF;
		int y0=(currentNode>>16) & 0xFFFF;

#define FINDPATH2_OUTPUT() \
	for(int i=0;i<possibleAdjacentCount;i++){ \
		if(xx==src.x+possibleAdjacentX[i] && yy==src.y+possibleAdjacentY[i]){ \
			FindPath2_Output(ret[i],x00,y00,xx,yy,bTempData,nTimeStamp); \
			nDestCount--; \
			break; \
		} \
	}

#define EXPAND_1(DX,DY,SKIP_FLAGS) \
	int xx=x0+DX,yy=y0+DY; \
	int idx=yy*m_nWidth+xx; \
	int d=bMoveableData[idx]; \
	int d2=bTempData[idx]-nTimeStamp; \
	switch(d & 0xFF){ \
	case SKIP_FLAGS: \
		if(d2<0){ \
			ValidateRotatingBlock(d>>8,bTempData,nTimeStamp); \
			d2=bTempData[idx]-nTimeStamp; \
		}

#define EXPAND_2(DIRECTION) \
		if(d2!=PATH_TIMESTAMP_NOT_BLOCKED || bMoveableData[idx]) break; \
		d2=bTempData[idx]-nTimeStamp; \
		/* fall through */ \
	case 0: \
		if(d2<0 || d2==PATH_TIMESTAMP_TARGET){ \
			bTempData[idx]=nTimeStamp+DIRECTION; \
			queuedNodes.push((yy<<16)|xx); \
			if(d2==PATH_TIMESTAMP_TARGET) FINDPATH2_OUTPUT(); \
		} \
		break; \
	default: \
		break; \
	}

		//left
		if(x0>0){
			EXPAND_1(-1,0,PATH_FINDING_HORIZONTAL_SKIP);
			xx--; idx--; assert(xx>=0);
			EXPAND_2(PATH_TIMESTAMP_LEFT);
		}

		//right
		if(x0<m_nWidth-1){
			EXPAND_1(1,0,PATH_FINDING_HORIZONTAL_SKIP);
			xx++; idx++; assert(xx<m_nWidth);
			EXPAND_2(PATH_TIMESTAMP_RIGHT);
		}

		//up
		if(y0>0){
			EXPAND_1(0,-1,PATH_FINDING_VERTICAL_SKIP);
			yy--; idx-=m_nWidth; assert(yy>=0);
			EXPAND_2(PATH_TIMESTAMP_UP);
		}

		//down
		if(y0<m_nHeight-1){
			EXPAND_1(0,1,PATH_FINDING_VERTICAL_SKIP);
			yy++; idx+=m_nWidth; assert(yy<m_nHeight);
			EXPAND_2(PATH_TIMESTAMP_DOWN);
		}
	}
}

#undef FINDPATH2_OUTPUT
#undef EXPAND_1
#undef EXPAND_2

void PuzzleBoyLevel::ValidateRotatingBlock(int index,unsigned int* bTempData,unsigned int nTimeStamp) const{
	const PushableBlock *objBlock=m_objBlocks[index];

	//check if there are no obstacles
	bool bHitTest=false;
	int m=(*objBlock)[0];
	int m2=m*(m+2); // (r+1)^2-1

	for(int yy=-m;yy<=m;yy++){
		for(int xx=-m;xx<=m;xx++){
			if(xx*xx+yy*yy<=m2){
				int xxx=xx+objBlock->m_x;
				int yyy=yy+objBlock->m_y;
				assert(xxx>=0 && xxx<m_nWidth && yyy>=0 && yyy<m_nHeight);

				int d=bTempData[yyy*m_nWidth+xxx]-nTimeStamp;
				if(d==PATH_TIMESTAMP_BLOCKED){
					bHitTest=true;
					break;
				}
			}
		}
		if(bHitTest) break;
	}

	assert(objBlock->m_x>=0 && objBlock->m_x<m_nWidth);
	if(true){
		int xx=objBlock->m_x;
		for(int y=-m;y<=m;y++){
			int yy=y+objBlock->m_y;
			assert(yy>=0 && yy<m_nHeight);

			int idx=yy*m_nWidth+xx;
			bTempData[idx]=nTimeStamp+((bHitTest || y==0)?PATH_TIMESTAMP_BLOCKED:PATH_TIMESTAMP_NOT_BLOCKED);
		}
	}
	assert(objBlock->m_y>=0 && objBlock->m_y<m_nHeight);
	if(true){
		int yy=objBlock->m_y;
		for(int x=-m;x<=m;x++){
			int xx=x+objBlock->m_x;
			assert(xx>=0 || xx<m_nWidth);

			int idx=yy*m_nWidth+xx;
			bTempData[idx]=nTimeStamp+((bHitTest || x==0)?PATH_TIMESTAMP_BLOCKED:PATH_TIMESTAMP_NOT_BLOCKED);
		}
	}
}

void PuzzleBoyLevel::FindPath2_Output(u8string& ret,int x0,int y0,int x1,int y1,const unsigned int* bTempData,unsigned int nTimeStamp) const{
	//found the path
	ret.clear();

	for(;x1!=x0 || y1!=y0;){
		//get previous coordinate
		int idx=y1*m_nWidth+x1;
		switch(bTempData[idx]-nTimeStamp){
		case PATH_TIMESTAMP_LEFT:
			x1++;
			if(x1<m_nWidth && (bTempData[idx+1]-nTimeStamp)==PATH_TIMESTAMP_NOT_BLOCKED) x1++;
			ret.push_back('A');
			break;
		case PATH_TIMESTAMP_RIGHT:
			x1--;
			if(x1>=0 && (bTempData[idx-1]-nTimeStamp)==PATH_TIMESTAMP_NOT_BLOCKED) x1--;
			ret.push_back('D');
			break;
		case PATH_TIMESTAMP_UP:
			y1++;
			if(y1<m_nHeight && (bTempData[idx+m_nWidth]-nTimeStamp)==PATH_TIMESTAMP_NOT_BLOCKED) y1++;
			ret.push_back('W');
			break;
		case PATH_TIMESTAMP_DOWN:
			y1--;
			if(y1>=0 && (bTempData[idx-m_nWidth]-nTimeStamp)==PATH_TIMESTAMP_NOT_BLOCKED) y1--;
			ret.push_back('S');
			break;
		default:
			assert(false);
			return;
		}
	}
}

//experimental.
//TODO: advanced optimize
void PuzzleBoyLevel::OptimizeRecord(u8string& rec)
{
	if(rec.empty()) return;

	std::vector<int> prev;
	std::vector<int> steps;
	std::vector<u8string> moves;
	std::map<RecordLevelChecksum,int> nodeMap;

	//init
	{
		prev.push_back(-1);
		steps.push_back(-1);
		moves.push_back(u8string());

		RecordLevelChecksum checksum;
		MySerializer ar;
		MySerialize(ar);

		//FIXME: ad-hoc workaround
		ar.PutInt32(m_nPlayerX);
		ar.PutInt32(m_nPlayerY);

		ar.CalculateBlake2s(&checksum,PuzzleBoyLevel::ChecksumSize);
		nodeMap[checksum]=0;
	}

	int prevNode=0;

	for(int i=0,m=rec.size();i<m;i++){
		int st=1;
		u8string currentMove;

		switch(rec[i]){
		case 'A':
		case 'a':
			if(MovePlayer(-1,0,false)) currentMove="A";
			break;
		case 'W':
		case 'w':
			if(MovePlayer(0,-1,false)) currentMove="W";
			break;
		case 'D':
		case 'd':
			if(MovePlayer(1,0,false)) currentMove="D";
			break;
		case 'S':
		case 's':
			if(MovePlayer(0,1,false)) currentMove="S";
			break;
		case '(':
			{
				int x=0,y=0;
				int i0=i;
				for(i++;i<m;i++){
					int ch=rec[i];
					if(ch>='0' && ch<='9'){
						x=x*10+ch-'0';
					}else if(ch==','){
						break;
					}
				}
				if(i>=m || x<=0 || x>m_nWidth) break;
				for(i++;i<m;i++){
					int ch=rec[i];
					if(ch>='0' && ch<='9'){
						y=y*10+ch-'0';
					}else if(ch==')'){
						break;
					}
				}
				if(i>=m || y<=0 || y>m_nHeight) break;
				//FIXME: CString::Mid?
				if(SwitchPlayer(x-1,y-1,false)) currentMove.assign(rec.begin()+i0,rec.begin()+(i+1));
				st=0;
			}
			break;
		default:
			break;
		}

		while(IsAnimating()) OnTimer();

		if(currentMove.empty()) continue;

		//calc steps
		st+=steps[prevNode];

		//calc checksum
		RecordLevelChecksum checksum;
		MySerializer ar;
		MySerialize(ar);

		//FIXME: ad-hoc workaround
		ar.PutInt32(m_nPlayerX);
		ar.PutInt32(m_nPlayerY);

		ar.CalculateBlake2s(&checksum,PuzzleBoyLevel::ChecksumSize);

		//check duplicates
		std::map<RecordLevelChecksum,int>::iterator it=nodeMap.find(checksum);
		if(it==nodeMap.end()){
			//not found, add new node
			prev.push_back(prevNode);
			steps.push_back(st);
			moves.push_back(currentMove);

			prevNode=prev.size()-1;
			nodeMap[checksum]=prevNode;
		}else{
			//chcek if it's better
			if(st<steps[it->second]){
				prev[it->second]=prevNode;
				steps[it->second]=st;
				moves[it->second]=currentMove;
			}

			prevNode=it->second;
		}

		if(IsWin()) break;
	}

	//get solution
	steps.clear();
	while(prevNode>=0){
		steps.push_back(prevNode);
		prevNode=prev[prevNode];
	}
	rec.clear();
	for(int i=steps.size()-1;i>=0;i--){
		rec.append(moves[steps[i]]);
	}
}

int PuzzleBoyLevel::SolveIt(u8string& rec,void* userData,int (*callback)(void*,LevelSolverState&)){
	//TODO: SolveIt
	return 0;
}
