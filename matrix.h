// matrix.h : main header file for the MATRIX application

#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CMatrixApp:

class CMatrixApp : public CWinApp
{
public:
	CMatrixApp();

	//{{AFX_VIRTUAL(CMatrixApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CMatrixApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
