// matrixDlg.cpp : implementation file

#include "stdafx.h"
#include "matrix.h"
#include "matrixDlg.h"

CMatrixDlg::CMatrixDlg(CWnd* pParent)
	: CDialog(CMatrixDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMatrixDlg)
	//}}AFX_DATA_INIT
}

void CMatrixDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMatrixDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMatrixDlg, CDialog)
	//{{AFX_MSG_MAP(CMatrixDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMatrixDlg message handlers

BOOL CMatrixDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CRect rect;
	GetDlgItem(IDC_MATRIX)->GetWindowRect(rect);
	ScreenToClient(rect);

	m_MatrixCtrl.Create(WS_VISIBLE | WS_CHILD, rect, this);

// adjustable parameters

//	m_MatrixCtrl.SetIntervals(10, 15);
//	m_MatrixCtrl.SetCursor(FALSE, FALSE);
//	m_MatrixCtrl.SetMaxPhaseCount(1000);
//	m_MatrixCtrl.AllowEuropeanChars(FALSE);

// text to display

	m_MatrixCtrl.AddTextLine("What is the Matrix?");
	m_MatrixCtrl.AddTextLine("\x01 2");					// 2 second delay
	m_MatrixCtrl.AddTextLine("You cannot be told");
	m_MatrixCtrl.AddTextLine("");
	m_MatrixCtrl.AddTextLine("You have to see for yourself...");
	m_MatrixCtrl.AddTextLine("\x01 2");					// 2 second delay
	m_MatrixCtrl.AddTextLine("Signal lock achieved");
	m_MatrixCtrl.AddTextLine("");
	m_MatrixCtrl.AddTextLine("Hold onto your chair!");
	m_MatrixCtrl.AddTextLine("\x01 10");				// 10 second delay
	m_MatrixCtrl.AddTextLine("Just kidding :)");

	return TRUE;
}
