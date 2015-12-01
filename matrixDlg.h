// matrixDlg.h : header file

#pragma once

#include "JMatrixCtrl.h"

class CMatrixDlg : public CDialog
{
public:

	CMatrixDlg(CWnd* pParent = NULL);

	//{{AFX_VIRTUAL(CMatrixDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

protected:

	//{{AFX_MSG(CMatrixDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:

	JMatrixCtrl	m_MatrixCtrl;

	//{{AFX_DATA(CMatrixDlg)
	enum { IDD = IDD_MATRIX_DIALOG };
	//}}AFX_DATA
};
