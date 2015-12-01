/*******************************************************************************
 JMatrixCtrl.h

	To use this class, simply create it and call AddTextLine() for each
	line of text to display.  You can specify a page break with a line
	that looks like "\x01 T", where T is a positive integer specifying
	how long to wait before starting on the next page.

	The default behavior is as close to the actual Matrix credits as I
	could manager.  You can obtain a range of different effects with
	the following settings:

	SetIntervals(const int intro, const int restart)

		Specifies how many seconds to wait before displaying the first
		page, and how long to wait before restarting after all pages
		have been displayed.

	SetCursor(const BOOL show, const BOOL solid)

		With no cursor, each line is drawn all at once and then begins
		to phase in.  Otherwise, the text appears as the cursor moves.
		If the cursor is not solid, it is a randomly changing character.

	SetMaxPhaseCount(const int maxCount)

		Specifies the maximum number of iterations before a character
		is forced to the correct value.  A character stops changing when
		it hits the correct value or exceeds maxCount.  Specify a huge
		value to let the characters phase in "naturally".  Specify zero
		to simply display the text.

	AllowEuropeanChars(const BOOL allow)

		The disadvantage of allowing European characters is that it takes
		*much* longer for the text to phase in when SetMaxPhaseCount()
		is called with a very large value.

	Written by John Lindal.
	http://jafl.my.speedingbits.com/

	Inspired by CMatrixCtrl written by Pablo van der Meer.
	http://www.pablovandermeer.nl/

	License:  Do what ever you want with it.
			  If you improve it, please let me know.

 *******************************************************************************/

#include "StdAfx.h"
#include "JMatrixCtrl.h"
#include <float.h>

// The following parameters can be tweaked to produce different effects.
// They are not included in the API because they are too obscure for the
// casual user.

const int kColSpacing          = 2;		// pixels between columns
const int kAnimateTextInterval = 10;	// milliseconds
const int kMoveCursorInterval  = 30;	// milliseconds
const int kAnimateBkgdInterval = 80;	// milliseconds
const float kSpinCharFraction  = 0.2f;	// fraction of columns with spinning character
const int kMinSpinCount        = 300;	// centiseconds
const int kMaxSpinCount        = 800;	// centiseconds

const COLORREF kTextColor      = RGB(128, 255, 128);
const int kBrightGreen         = 255;	// out of 255
const int kMinGreen            = 75;	// out of 255
const int kMaxGreen            = 150;	// out of 255

/*
const COLORREF kTextColor      = RGB(0, 255, 0);
const int kBrightGreen         = 210;	// out of 255
const int kMinGreen            = 60;	// out of 255
const int kMaxGreen            = 100;	// out of 255
*/

const unsigned char kMinBackChar = 32;
const unsigned char kMaxBackChar = '\xFF';

const char kPageBreak          = '\x01';
const char kBlockCursorChar    = '\x01';

enum
{
	kInitTextID,
	kUpdateTextID,
	kUpdateCursorID,
	kUpdateBackgroundID,
	kUpdateSpinID		// only runs when kUpdateTextID does not run to minimize redraws
};

#define getrandom(min,max) ((rand()%(int)(((max)+1)-(min)))+(min))

/*******************************************************************************
 Constructor

 *******************************************************************************/
 
JMatrixCtrl::JMatrixCtrl()
	:
	m_MaxChar('\xFF'),
	m_IntroInterval(5),
	m_RestartInterval(5),
	m_nMaxPhaseCount(20),
	m_bShowCursor(TRUE),
	m_CursorPt(-1, -1),
	m_CursorChar(kBlockCursorChar),
	m_nPageStartLine(0),
	m_nActiveLine(-1),
	m_pFontOld(NULL),
	m_pBitmapOld(NULL),
	m_pMatrixColumns(NULL),
	m_nActiveColumns(0),
	m_pSpinChars(NULL),
	m_nActiveSpins(0),
	m_nTotalSpins(0),
	m_pBackFontOld(NULL),
	m_pBackBitmapOld(NULL)
{
	srand((unsigned int) time(NULL));
}

/*******************************************************************************
 Destructor

 *******************************************************************************/

JMatrixCtrl::~JMatrixCtrl()
{
	if (m_pBitmapOld != NULL)
		{
		m_DC.SelectObject(m_pBitmapOld);  
		}
	if (m_pFontOld != NULL)
		{
		m_DC.SelectObject(m_pFontOld);  
		}

	if (m_pBackBitmapOld != NULL)
		{
		m_BackDC.SelectObject(m_pBackBitmapOld);  
		}
	if (m_pBackFontOld != NULL)
		{
		m_BackDC.SelectObject(m_pBackFontOld);  
		}

	delete [] m_pMatrixColumns;
	delete [] m_pSpinChars;
}

/*******************************************************************************
 Create

 *******************************************************************************/

BOOL
JMatrixCtrl::Create
	(
	DWORD		dwStyle,
	const RECT&	rect, 
	CWnd*		pParentWnd,
	UINT		nID
	)
{
	static CString className = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW);

	const BOOL result = CreateEx(WS_EX_CLIENTEDGE, className, NULL, dwStyle, rect,
								 pParentWnd, nID);

	CRect r;
	GetClientRect(r);
	const int h = r.Height();
	const int w = r.Width();

	m_Font.CreateFont(14, 0, 0, 0, FW_BOLD,
					  FALSE, FALSE, 0, ANSI_CHARSET,
					  OUT_DEFAULT_PRECIS, 
					  CLIP_DEFAULT_PRECIS,
					  DEFAULT_QUALITY, 
					  DEFAULT_PITCH|FF_SWISS, "Courier");

	m_BGFont.CreateFont(14, 0, 0, 0, FW_BOLD,
						FALSE, FALSE, 0, GREEK_CHARSET,
						OUT_DEFAULT_PRECIS, 
						CLIP_DEFAULT_PRECIS,
						DEFAULT_QUALITY, 
						DEFAULT_PITCH|FF_SWISS, "Courier");

	// create main DC that gets copied to window

	CClientDC dc(this);
	m_DC.CreateCompatibleDC(&dc);
	m_Bitmap.CreateCompatibleBitmap(&dc, w, h);
	m_pBitmapOld = m_DC.SelectObject(&m_Bitmap);
	m_pFontOld   = m_DC.SelectObject(&m_Font);

	TEXTMETRIC tm;
	m_DC.GetTextMetrics(&tm);
	m_nTextWidth = tm.tmAveCharWidth + kColSpacing;
	m_nTextHeight= tm.tmHeight;
	m_nCols      = w/m_nTextWidth  + 1;
	m_nRows      = h/m_nTextHeight + 1;

	// create background DC that stores rain animation

	m_BackDC.CreateCompatibleDC(&dc);
	m_BackBitmap.CreateCompatibleBitmap(&dc, w, h);
	m_pBackBitmapOld = m_BackDC.SelectObject(&m_BackBitmap);
	m_pBackFontOld   = m_BackDC.SelectObject(&m_BGFont);

	m_BackDC.FillSolidRect(0,0, w,h, RGB(0,0,0));

	m_pMatrixColumns = new MatrixColumn[ m_nCols ];

	for (int i=0; i<m_nCols; i++)
		{
		m_pMatrixColumns[i].bActive = FALSE;
		}

	m_nTotalSpins = (int) (m_nCols * kSpinCharFraction);
	m_pSpinChars  = new SpinChar[ m_nTotalSpins ];

	for (i=0; i<m_nTotalSpins; i++)
		{
		m_pSpinChars[i].bActive = FALSE;
		}

	SetTimer(kInitTextID, m_IntroInterval * 1000, NULL);
	SetTimer(kUpdateBackgroundID, kAnimateBkgdInterval, NULL);

	Invalidate(FALSE);
	return result;
}

/*******************************************************************************
 SetCursor

	If the cursor is not solid, it is a randomly changing character.

 *******************************************************************************/

void
JMatrixCtrl::SetCursor
	(
	const BOOL show,
	const BOOL solid
	)
{
	m_bShowCursor = show;
	m_CursorChar  = (solid ? kBlockCursorChar : 'a');
}

/*******************************************************************************
 Message map

 *******************************************************************************/

BEGIN_MESSAGE_MAP(JMatrixCtrl, CWnd)
	//{{AFX_MSG_MAP(JMatrixCtrl)
	ON_WM_PAINT()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*******************************************************************************
 OnPaint

 *******************************************************************************/

void
JMatrixCtrl::OnPaint()
{
	if (m_pBitmapOld != NULL)
		{
		CRect r;
		GetClientRect(&r);

		CPaintDC dc(this);
		dc.BitBlt(0, 0, r.Width(), r.Height(), &m_DC, 0, 0, SRCCOPY);
		}
}

/*******************************************************************************
 OnTimer

 *******************************************************************************/

void
JMatrixCtrl::OnTimer
	(
	UINT nEventID
	)
{
	if (nEventID == kInitTextID)
		{
		InitText();
		InitCursor();
		}
	else if (nEventID == kUpdateTextID)
		{
		UpdateSpin();
		UpdateText();
		Draw();
		}
	else if (nEventID == kUpdateCursorID)
		{
		UpdateCursor();
		Draw();
		}
	else if (nEventID == kUpdateBackgroundID)
		{
		UpdateBackground();
		Draw();
		}
	else if (nEventID == kUpdateSpinID)		// runs when kUpdateTextID is not active
		{
		UpdateSpin();
		Draw();
		}

	CWnd::OnTimer(nEventID);
}

/*******************************************************************************
 Draw (private)

 *******************************************************************************/

void
JMatrixCtrl::Draw()
{
	CRect r;
	GetClientRect(&r);

	m_DC.BitBlt(0, 0, r.Width(), r.Height(), &m_BackDC, 0, 0, SRCCOPY);
	DrawSpin();
	DrawText();
	DrawCursor();
	Invalidate(FALSE);
}

/*******************************************************************************
 InitText (private)

 *******************************************************************************/

void
JMatrixCtrl::InitText()
{
	if (m_LineList.GetSize() == 0)
		{
		return;
		}

	m_nPageStartLine = m_nPageEndLine + 2;		// skip page break
	if (m_nPageStartLine >= m_LineList.GetSize())
		{
		m_nPageStartLine = 0;
		}
	m_nActiveLine = m_nPageEndLine = m_nPageStartLine;

	while (1)
		{
		const CString& line = m_LineList.ElementAt(m_nPageEndLine);
		if (!line.IsEmpty() && line[0] == kPageBreak)
			{
			CString s = line;
			s.Delete(0, 1);
			s.TrimLeft();
			m_nPauseInterval = atoi(s);
			m_nPageEndLine--;
			break;
			}
		else if (m_nPageEndLine == m_LineList.GetSize()-1)
			{
			m_nPauseInterval = m_RestartInterval;
			break;
			}

		m_nPageEndLine++;
		}

	const int lineCount = m_nPageEndLine - m_nPageStartLine + 1;
	const int topLine   = (m_nRows-1 - lineCount)/2;

	CRect r;
	GetClientRect(&r);

	m_LineStartList.RemoveAll();
	for (int i=0; i<lineCount; i++)
		{
		const CString& line = m_LineList.ElementAt(m_nPageStartLine + i);
		const CSize size    = m_DC.GetTextExtent(line, line.GetLength());

		CPoint pt;
		pt.x = ((r.Width() - size.cx)/2)/m_nTextWidth;
		pt.y = topLine + i;
		m_LineStartList.Add(pt);
		}

	m_ActiveLine.Empty();
	m_PhaseList.RemoveAll();
	KillTimer(kInitTextID);
	KillTimer(kUpdateSpinID);
	SetTimer(kUpdateTextID, kAnimateTextInterval, NULL);
}

/*******************************************************************************
 UpdateText (private)

 *******************************************************************************/

void
JMatrixCtrl::UpdateText()
{
	if (m_bShowCursor && m_CursorChar != kBlockCursorChar)
		{
		m_CursorChar = (char) getrandom(32, m_MaxChar);
		}

	BOOL done = TRUE;

	const CString& line  = m_LineList.ElementAt(m_nActiveLine);
	const int lineLength = line.GetLength();
	for (int i=0; i<lineLength; i++)
		{
		if (m_bShowCursor)
			{
			const CPoint& pt = m_LineStartList.ElementAt(m_nActiveLine - m_nPageStartLine);
			if (i >= m_CursorPt.x - pt.x)
				{
				done = FALSE;
				break;
				}
			}

		if (m_ActiveLine.GetLength() <= i)
			{
			m_ActiveLine += (char) getrandom(32, m_MaxChar);
			m_PhaseList.Add(0);
			done = FALSE;
			}
		else if (m_PhaseList[i] >= m_nMaxPhaseCount)
			{
			m_ActiveLine.SetAt(i, line[i]);
			}
		else if (m_ActiveLine[i] != line[i])
			{
			m_ActiveLine.SetAt(i, (char) getrandom(32, m_MaxChar));
			(m_PhaseList.ElementAt(i))++;
			done = FALSE;
			}
		}

	if (m_bShowCursor && lineLength > 0 && !CursorFinished())
		{
		done = FALSE;
		}

	if (done && m_nActiveLine == m_nPageEndLine)
		{
		KillTimer(kUpdateTextID);
		SetTimer(kInitTextID, m_nPauseInterval * 1000, NULL);
		SetTimer(kUpdateSpinID, kAnimateTextInterval, NULL);
		}
	else if (done)
		{
		m_nActiveLine++;
		m_ActiveLine.Empty();
		m_PhaseList.RemoveAll();
		InitCursor();
		}
}

/*******************************************************************************
 DrawText (private)

 *******************************************************************************/

void
JMatrixCtrl::DrawText()
{
	for (int i=m_nPageStartLine; i<m_nActiveLine; i++)
		{
		const CPoint& pt    = m_LineStartList.ElementAt(i - m_nPageStartLine);
		const CString& line = m_LineList.ElementAt(i);
		DrawActiveString(m_DC, pt.y, pt.x, line, line.GetLength(), kTextColor);
		}

	if (m_nActiveLine >= 0)
		{
		const CPoint& pt = m_LineStartList.ElementAt(m_nActiveLine - m_nPageStartLine);
		DrawActiveString(m_DC, pt.y , pt.x, m_ActiveLine, m_ActiveLine.GetLength(), kTextColor);
		}
}

/*******************************************************************************
 InitCursor

 *******************************************************************************/

void
JMatrixCtrl::InitCursor()
{
	if (m_bShowCursor && m_nActiveLine >= 0)
		{
		const CPoint& pt = m_LineStartList.ElementAt(m_nActiveLine - m_nPageStartLine);
		m_CursorPt.x       = 0;
		m_CursorPt.y       = pt.y;
		SetTimer(kUpdateCursorID, kMoveCursorInterval, NULL);
		}
}

/*******************************************************************************
 UpdateCursor

 *******************************************************************************/

void
JMatrixCtrl::UpdateCursor()
{
	m_CursorPt.x++;
	if (CursorFinished())
		{
		KillTimer(kUpdateCursorID);
		}
}

/*******************************************************************************
 DrawCursor (private)

 *******************************************************************************/

void
JMatrixCtrl::DrawCursor()
{
	if (m_bShowCursor)
		{
		DrawActiveString(m_DC, m_CursorPt.y, m_CursorPt.x, &(m_CursorChar), 1, kTextColor);
		}
}

/*******************************************************************************
 InitBackgroundCharacters (private)

 *******************************************************************************/

void
JMatrixCtrl::InitBackgroundCharacters
	(
	const int col
	)
{
	const int bottomOffset = 3;

	m_pMatrixColumns[col].nCounter = 0;
	if (getrandom(1,2) == 1)
		{
		m_pMatrixColumns[col].nCounter = getrandom(0, m_nRows-bottomOffset);
		}

	m_pMatrixColumns[col].nCounterMax = m_nRows;
	if (getrandom(1,2) == 1)
		{
		m_pMatrixColumns[col].nCounterMax =
			m_pMatrixColumns[col].nCounter +
			getrandom(bottomOffset, m_nRows-m_pMatrixColumns[col].nCounter);
		if (m_pMatrixColumns[col].nCounterMax > m_nRows)	// don't trust getrandom()
			{
			m_pMatrixColumns[col].nCounterMax = m_nRows;
			}
		}

	const BOOL blank           = (getrandom(1,5) == 1);
	m_pMatrixColumns[col].prev = (blank ? ' ' : getrandom(kMinBackChar, kMaxBackChar));
}

/*******************************************************************************
 UpdateBackground

 *******************************************************************************/

void
JMatrixCtrl::UpdateBackground()
{
	// activate another column

	if (m_nActiveColumns < m_nCols)
		{
		int nStartColumn, nSafetyCounter = 0;
		do
			{
			nStartColumn = rand() % m_nCols;
			nSafetyCounter++;
			if (nSafetyCounter > m_nCols)
				break;
			}
			while (m_pMatrixColumns[nStartColumn].bActive);

		if (!m_pMatrixColumns[nStartColumn].bActive)
			{
			m_pMatrixColumns[nStartColumn].bActive = TRUE;
			InitBackgroundCharacters(nStartColumn);
			DrawActiveBackgroundChar(nStartColumn);

			m_pMatrixColumns[nStartColumn].nCounter++;
			m_nActiveColumns++;
			}
		}

	// increment each active column

	for (int i=0; i<m_nCols; i++)
		{
		if (m_pMatrixColumns[i].bActive &&
			m_pMatrixColumns[i].nCounter >= m_pMatrixColumns[i].nCounterMax)
			{
			DrawFadedBackgroundChar(i);

			m_pMatrixColumns[i].bActive = FALSE;
			m_nActiveColumns--;
			}
		else if (m_pMatrixColumns[i].bActive)
			{
			DrawFadedBackgroundChar(i);
			DrawActiveBackgroundChar(i);

			m_pMatrixColumns[i].nCounter++;
			}
		}
}

/*******************************************************************************
 DrawActiveBackgroundChar (private)

	Must be called after DrawFadedBackgroundChar() since it replaces prev.

 *******************************************************************************/

void
JMatrixCtrl::DrawActiveBackgroundChar
	(
	const int col
	)
{
	if (m_pMatrixColumns[col].prev != ' ')
		{
		m_pMatrixColumns[col].prev = getrandom(kMinBackChar, kMaxBackChar);
		}

	const int row = m_pMatrixColumns[col].nCounter;
	DrawActiveString(m_BackDC, row, col, &(m_pMatrixColumns[col].prev), 1,
					 RGB(0, kBrightGreen, 0));
}

/*******************************************************************************
 DrawFadedBackgroundChar (private)

 *******************************************************************************/

void
JMatrixCtrl::DrawFadedBackgroundChar
	(
	const int col
	)
{
	const int row    = m_pMatrixColumns[col].nCounter - 1;
	const CSize size = m_BackDC.GetTextExtent(&(m_pMatrixColumns[col].prev), 1);

	m_BackDC.SetTextColor(RGB(0, getrandom(kMinGreen, kMaxGreen), 0));
	m_BackDC.TextOut(col * m_nTextWidth + (m_nTextWidth - size.cx)/2,
					 row * m_nTextHeight,
					 &(m_pMatrixColumns[col].prev), 1);
}

/*******************************************************************************
 UpdateSpin

 *******************************************************************************/

void
JMatrixCtrl::UpdateSpin()
{
	// activate another spinning character

	if (m_nActiveSpins < m_nTotalSpins && getrandom(0,100) == 0)
		{
		int nIndex, nSafetyCounter = 0;
		do
			{
			nIndex = rand() % m_nTotalSpins;
			nSafetyCounter++;
			if (nSafetyCounter > m_nTotalSpins)
				break;
			}
			while (m_pSpinChars[nIndex].bActive);

		if (!m_pSpinChars[nIndex].bActive)
			{
			m_pSpinChars[nIndex].bActive  = TRUE;
			m_pSpinChars[nIndex].nCounter = getrandom(kMinSpinCount, kMaxSpinCount);
			m_pSpinChars[nIndex].pt.x     = getrandom(0, m_nCols);
			m_pSpinChars[nIndex].pt.y     = getrandom(0, m_nRows);

			m_nActiveSpins++;
			}
		}

	// increment each spinning character

	for (int i=0; i<m_nTotalSpins; i++)
		{
		if (m_pSpinChars[i].bActive &&
			m_pSpinChars[i].nCounter <= 0)
			{
			m_pSpinChars[i].bActive = FALSE;
			m_nActiveSpins--;
			}
		else if (m_pSpinChars[i].bActive)
			{
			m_pSpinChars[i].c = getrandom(kMinBackChar, kMaxBackChar);
			m_pSpinChars[i].nCounter--;
			}
		}
}

/*******************************************************************************
 DrawSpin

 *******************************************************************************/

void
JMatrixCtrl::DrawSpin()
{
	for (int i=0; i<m_nTotalSpins; i++)
		{
		if (m_pSpinChars[i].bActive)
			{
			DrawActiveString(m_DC, m_pSpinChars[i].pt.y, m_pSpinChars[i].pt.x,
							 &(m_pSpinChars[i].c), 1, RGB(0, kBrightGreen, 0));
			}
		}
}

/*******************************************************************************
 DrawActiveString (private)

 *******************************************************************************/

void
JMatrixCtrl::DrawActiveString
	(
	CDC&			dc,
	const int		row,
	const int		col,
	const char*		str,
	const int		len,
	const COLORREF	color
	)
{
	if (len > 0)
		{
		const CSize size = dc.GetTextExtent(str, len);

		CRect r(      col*m_nTextWidth,     row*m_nTextHeight,
				(col+len)*m_nTextWidth, (row+1)*m_nTextHeight);
		if (len > 1)
			{
			r.right = r.left + size.cx;
			}

		if (len == 1 && str[0] == kBlockCursorChar)
			{
			dc.FillSolidRect(r, color);
			}
		else
			{
			dc.FillSolidRect(r, RGB(0,0,0));

			dc.SetTextColor(color);
			dc.TextOut(col * m_nTextWidth + (len > 1 ? 0 : (m_nTextWidth - size.cx)/2),
					   row * m_nTextHeight,
					   str, len);
			}
		}
}
