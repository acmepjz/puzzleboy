#pragma once

#include "PuzzleBoyLevelData.h"
#include "MySerializer.h"

class PushableBlock;
class PuzzleBoyLevelUndo;
struct BlockBFSNode;

struct LevelSolverState{
	int nAllNodeCount;
	int nOpenedNodeCount;
};

//return value: non-zero=abort (?)
typedef int (*LevelSolverCallback)(void* userData,const LevelSolverState& progress);

class VertexList;
struct PuzzleBoyLevelGraphics;
class PuzzleBoyLevelView;

class PuzzleBoyLevel: public PuzzleBoyLevelData
{
public:
	//options of UpdateGraphics
	static const int UpdateEmptyTile=1<<EMPTY_TILE;
public:
	PuzzleBoyLevel();
	PuzzleBoyLevel(const PuzzleBoyLevelData& obj);
	virtual ~PuzzleBoyLevel();

	void StartGame();

	void CreateGraphics();
	void DestroyGraphics();
	void UpdateGraphics(int type);
	void Draw(bool bEditMode=false,int nEditingBlockIndex=-1);

	//internal functions
	//nValue<0 means any non-zero value
	static void CreateRectangles(int nLeft,int nTop,int nWidth,int nHeight,const unsigned char* bData,int nValue,VertexList *v);
	//nValue<0 means any non-zero value
	static void CreateBorder(int nLeft,int nTop,int nWidth,int nHeight,const unsigned char* bData,int nValue,VertexList *shade,VertexList *lines,float fSize);

	bool IsAnimating() const;
	bool IsWin() const;
	bool IsTargetFinished() const {return m_nTargetCount==m_nTargetFinished;}
	bool MovePlayer(int dx,int dy,bool bSaveUndo,int nRecordIndex=-1);
	bool SwitchPlayer(int x,int y,bool bSaveUndo,int nRecordIndex=-1);

	//animationTime should be 0,1,2,4 or 8
	void OnTimer(int animationTime=0);

	//find a path for player from one position to another position
	bool FindPath(u8string& ret,int x1,int y1,int x0=-1,int y0=-1) const;

	//find a path for a box from one position to another position
	//warning: no sanity check!
	bool FindPathForBlock(u8string& ret,int index,int x1,int y1) const;

	//experimental function.
	//rec: the input and output record
	//note: call StartGame() before calling this function
	void OptimizeRecord(u8string& rec);

	//highly experimental function.
	//rec: the output record
	//return value: 1=success 0=no solution -1=abort -2=error
	//note: call StartGame() before calling this function
	int SolveIt(u8string& rec,void* userData,LevelSolverCallback callback) const;

	bool CanUndo() const {return m_nCurrentUndo>0 && m_nCurrentUndo<=(int)m_objUndo.size();}
	bool CanRedo() const {return m_nCurrentUndo>=0 && m_nCurrentUndo<(int)m_objUndo.size();}
	bool Undo();
	bool Redo();

	bool UndoToRecordIndex(int nRecordIndex,int* ret=0);

	void SaveUndo(PuzzleBoyLevelUndo* objUndo);

	//mode: 0=normal 1=get all record including redo history 2=redo history only
	u8string GetRecord(int mode=0) const;
	//rec: the record
	//redoHistory: only save record to redo history
	//start: the start index
	//end: the end index (not including this index)
	//ret_end: returns the actual end index. only valid when this function returns true
	bool ApplyRecord(const u8string& rec,bool redoHistory=false,int start=0,int end=-1,int* ret_end=0);

	void SerializeHistory(MySerializer& ar);

	int GetCurrentPlayerX() const {return m_nPlayerX;}
	int GetCurrentPlayerY() const {return m_nPlayerY;}

	//ad-hoc function, no sanity check
	bool CheckIfCurrentPlayerCanMove(int dx,int dy,int x0=-1,int y0=-1);
private:
	//internal function
	void FindPath2(const PushableBlock *targetBlock,const BlockBFSNode& src,const int* possibleAdjacentX,const int* possibleAdjacentY,int possibleAdjacentCount,const unsigned int* bMoveableData,unsigned int* bTempData,unsigned int nTimeStamp,u8string* ret) const;
	//internal function
	void FindPath2_Output(u8string& ret,int x0,int y0,int x1,int y1,const unsigned int* bTempData,unsigned int nTimeStamp) const;
	//internal function for path finding
	void ValidateRotatingBlock(int index,unsigned int* bTempData,unsigned int nTimeStamp) const;

	void UpdateMoveableData();

public:
	bool m_bSendNetworkMove;
	int m_nMoves;
	std::vector<unsigned char> m_bMoveableData,m_bTargetData;
	int m_nPlayerCount;
	int m_nTargetCount,m_nTargetFinished;
	int m_nExitCount;

	unsigned char GetMoveableData(int x,int y) const {return m_bMoveableData[y*m_nWidth+x];}
	unsigned char GetTargetData(int x,int y) const {return m_bTargetData[y*m_nWidth+x];}
private:
	int m_nPlayerX,m_nPlayerY;
	int m_nMoveAnimationX,m_nMoveAnimationY,m_nMoveAnimationTime;
	int m_nBlockAnimationIndex,m_nBlockAnimationFlags;
	int m_nSwitchAnimationTime;
	int m_nExitAnimationTime;

	std::vector<PuzzleBoyLevelUndo*> m_objUndo;
	int m_nCurrentUndo; //point to current state, 0 to m_objUndo.GetSize()
	PuzzleBoyLevelUndo* m_objCurrentUndo;

public:
	PuzzleBoyLevelGraphics *m_Graphics;
	PuzzleBoyLevelView *m_view; //weak reference!!! don't delete it!!!
};

#define PUSH_BLOCK_COLOR 1.0f,1.0f,0.0f,1.0f
#define PUSH_BLOCK_COLOR_trans 1.0f,1.0f,0.0f,0.5f

extern const float PUSH_BLOCK_COLOR_trans_mat[4];
extern const float PUSH_BLOCK_COLOR_mat[4];

#define TARGET_BLOCK_COLOR 0.0f,1.0f,0.0f,1.0f
#define TARGET_BLOCK_COLOR_trans 0.0f,1.0f,0.0f,0.5f

extern const float TARGET_BLOCK_COLOR_mat[4];
extern const float TARGET_BLOCK_COLOR_trans_mat[4];
