#ifndef _SPIDER_PCH_
#define _SPIDER_PCH_

#include <cardlib.h>

#include "resource.h"

#define DIFFICULTY_ONE_COLOR     1
#define DIFFICULTY_TWO_COLORS    2
#define DIFFICULTY_FOUR_COLORS   3
#define CARDBACK_START           IDC_CARDBACK1
#define CARDBACK_END             IDC_CARDBACK12
#define NUM_CARDBACKS            (CARDBACK_END - CARDBACK_START + 1)
#define CARDBACK_RES_START       53
#define CARDBACK_OPTIONS_WIDTH   72
#define CARDBACK_OPTIONS_HEIGHT  112

#define X_BORDER                  4
#define Y_BORDER                  4

#define NUM_STACKS         10

extern HWND hwndMain;
extern CardWindow SpiderWnd;
extern TCHAR szAppName[];
extern bool fGameStarted;
extern int yRowStackCardOffset;
extern DWORD dwDifficulty;
extern TCHAR MsgDeal[];
extern TCHAR MsgWin[];

void CreateSpider(void);
void NewGame(void);

bool CARDLIBPROC RowStackDragProc(CardRegion &stackobj, int iNumCards);
bool CARDLIBPROC RowStackDropProc(CardRegion &stackobj,  const CardStack &dragcards);

void CARDLIBPROC RowStackClickProc(CardRegion &stackobj, int iNumClicked);

void CARDLIBPROC DeckClickProc(CardRegion &stackobj, int iNumClicked);
void CARDLIBPROC PileDblClickProc(CardRegion &stackobj, int iNumClicked);

void CARDLIBPROC PileRemoveProc(CardRegion &stackobj, int iRemoved);

#endif /* _SPIDER_PCH_ */
