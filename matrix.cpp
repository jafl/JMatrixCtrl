// matrix.cpp : Defines the class behaviors for the application.

#include "stdafx.h"
#include "matrix.h"
#include "matrixDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CMatrixApp

BEGIN_MESSAGE_MAP(CMatrixApp, CWinApp)
	//{{AFX_MSG_MAP(CMatrixApp)
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMatrixApp construction

CMatrixApp::CMatrixApp()
{
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CMatrixApp object

CMatrixApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CMatrixApp initialization

BOOL CMatrixApp::InitInstance()
{
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	CMatrixDlg dlg;
	m_pMainWnd = &dlg;
	dlg.DoModal();

_CrtDumpMemoryLeaks();

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
