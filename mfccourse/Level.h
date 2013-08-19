#pragma once

#include "MySerializer.h"

// CLevel ÃüÁîÄ¿±ê

#define FLOOR_TILE 0
#define WALL_TILE 1
#define EMPTY_TILE 2
#define TARGET_TILE 3
#define EXIT_TILE 4
#define PLAYER_TILE 5

//----------------(MFC) file ver 2

//edit mode only
#define PLAYER_AND_TARGET_TILE 6

//serialized form only

//all these blocks are 1x1
#define NORMAL_BLOCK_TILE 7
#define TARGET_BLOCK_TILE 8
#define TARGET_BLOCK_ON_TARGET_TILE 9
//format: RLE_TILE n T, n+4=tile count
//additional format in 4-bit file: RLE_TILE 0xF n n n T, n+37=tile count
#define RLE_TILE 15

//----------------

#define FLOOR_COLOR 0xFFC0C0

#define BLOCK_COLOR 0x0040FF
#define BLOCK_LIGHT_COLOR 0x0080FF
#define BLOCK_DARK_COLOR 0x0000FF

#define PUSH_BLOCK_COLOR 0x00FFFF
#define PUSH_BLOCK_LIGHT_COLOR 0x80FFFF
#define PUSH_BLOCK_DARK_COLOR 0x0080FF

#define ROTATE_BLOCK_COLOR 0xFFFF00

#define TARGET_BLOCK_COLOR 0x00FF00
#define TARGET_BLOCK_LIGHT_COLOR 0x80FF80
#define TARGET_BLOCK_DARK_COLOR 0x00C000

#define TARGET_COLOR 0x008000

class CPushableBlock;
class CLevelUndo;
struct BlockBFSNode;

struct LevelSolverState{
	int nCurrentNodeCount;
	int nOpenedNodeCount;
};

class CLevel : public CObject
{
public:
	DECLARE_SERIAL(CLevel);

	CLevel();
	CLevel(const CLevel& obj);
	virtual ~CLevel();

	void Serialize(CArchive& ar);

	void MySerialize(MySerializer& ar);

	void Create(int nWidth,int nHeight,int bPreserve=0,int nXOffset=0,int nYOffset=0);
	void CreateDefault();

	void StartGame();

	int GetBlockSize(int nWidth,int nHeight) const;
	RECT GetDrawArea(RECT r,int nBlockSize=0) const;
	void Draw(CDC *pDC,RECT r,bool bEditMode,bool bCentering,int nBlockSize,int nEditingBlockIndex,bool bDrawGrid) const;

	int HitTestForPlacingBlocks(int x,int y,int nEditingBlockIndex=-1) const;

	bool IsAnimating() const;
	bool IsWin() const;
	bool MovePlayer(int dx,int dy,bool bSaveUndo);
	bool SwitchPlayer(int x,int y,bool bSaveUndo);
	void OnTimer();

	//find a path for player from one position to another position
	bool FindPath(CString& ret,int x1,int y1,int x0=-1,int y0=-1) const;

	//find a path for a box from one position to another position
	//warning: no sanity check!
	bool FindPathForBlock(CString& ret,int index,int x1,int y1) const;

	//experimental function.
	//rec: the input and output record
	//note: call StartGame() before calling this function
	void OptimizeRecord(CString& rec);

	//highly experimental function.
	//rec: the output record
	//return value: 1=success 0=failed -1=error or abort
	//note: call StartGame() before calling this function
	int SolveIt(CString& rec,void* userData,int (*callback)(void*,LevelSolverState&));

	bool CanUndo() const {return m_nCurrentUndo>0 && m_nCurrentUndo<=m_objUndo.GetSize();}
	bool CanRedo() const {return m_nCurrentUndo>=0 && m_nCurrentUndo<m_objUndo.GetSize();}
	bool Undo();
	bool Redo();

	void SaveUndo(CLevelUndo* objUndo);

	CString GetRecord();
	bool ApplyRecord(const CString& rec);

	unsigned char operator()(int x,int y) const {return m_bMapData[y*m_nWidth+x];}
	unsigned char& operator()(int x,int y) {return m_bMapData[y*m_nWidth+x];}
	unsigned char operator[](int idx) const {return m_bMapData[idx];}
	unsigned char& operator[](int idx) {return m_bMapData[idx];}

	int GetCurrentPlayerX() const {return m_nPlayerX;}
	int GetCurrentPlayerY() const {return m_nPlayerY;}

	//ad-hoc function, no sanity check
	bool CheckIfCurrentPlayerCanMove(int dx,int dy,int x0=-1,int y0=-1);

	static void DrawEdgedBlock(CDC *pDC,RECT r,int nEdgeSize,CBrush *hbr,CBrush *hbrLight,CBrush *hbrDark,const bool bNeighborhood[8]);
private:
	//internal function
	void FindPath2(const CPushableBlock *targetBlock,const BlockBFSNode& src,const int* possibleAdjacentX,const int* possibleAdjacentY,int possibleAdjacentCount,const unsigned int* bMoveableData,unsigned int* bTempData,unsigned int nTimeStamp,CString* ret) const;
	//internal function
	void FindPath2_Output(CString& ret,int x0,int y0,int x1,int y1,const unsigned int* bTempData,unsigned int nTimeStamp) const;
	//internal function for path finding
	void ValidateRotatingBlock(int index,unsigned int* bTempData,unsigned int nTimeStamp) const;

	void UpdateMoveableData();
public:
	CString m_sLevelName;
	static const int ChecksumSize=160/8;
	int m_nWidth;
	int m_nHeight;
	CArray<unsigned char> m_bMapData;
	CObArray m_objBlocks;
public:
	int m_nMoves;
private:
	CArray<unsigned char> m_bMoveableData,m_bTargetData;
	int m_nPlayerCount;
	int m_nPlayerX,m_nPlayerY;
	int m_nMoveAnimationX,m_nMoveAnimationY,m_nMoveAnimationTime;
	int m_nBlockAnimationIndex,m_nBlockAnimationFlags;
	int m_nSwitchAnimationTime;
	int m_nTargetCount,m_nTargetFinished;
	int m_nExitCount;

	CObArray m_objUndo;
	int m_nCurrentUndo; //point to current state, 0 to m_objUndo.GetSize()
	CLevelUndo* m_objCurrentUndo;
};


