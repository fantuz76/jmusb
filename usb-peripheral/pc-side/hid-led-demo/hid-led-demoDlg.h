/****************************************************************************
 *
 *            Copyright (c) 2006-2007 by CMX Systems, Inc.
 *
 * This software is copyrighted by and is the sole property of
 * CMX.  All rights, title, ownership, or other interests
 * in the software remain the property of CMX.  This
 * software may only be used in accordance with the corresponding
 * license agreement.  Any unauthorized use, duplication, transmission,
 * distribution, or disclosure of this software is expressly forbidden.
 *
 * This Copyright notice may not be removed or modified without prior
 * written consent of CMX.
 *
 * CMX reserves the right to modify this software without notice.
 *
 * CMX Systems, Inc.
 * 12276 San Jose Blvd. #511
 * Jacksonville, FL 32223
 * USA
 *
 * Tel:  (904) 880-1840
 * Fax:  (904) 880-1632
 * http: www.cmx.com
 * email: cmx@cmx.com
 *
 ***************************************************************************/
// hid-led-demoDlg.h : header file
//

#pragma once
#include "afxwin.h"


// ChidleddemoDlg dialog
class ChidleddemoDlg : public CDialog
{
private:
  UINT_PTR timer;
// Construction
public:
	ChidleddemoDlg(CWnd* pParent = NULL);	// standard constructor
  
// Dialog Data
	enum { IDD = IDD_HIDLEDDEMO_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnBnClickedCheck1();
  CButton cbLed1;
  CButton cbLed2;
  CButton cbLed3;
  CButton cbLed4;
  afx_msg void OnBnClickedCheck2();
  afx_msg void OnBnClickedCheck3();
  afx_msg void OnBnClickedCheck4();
  CButton cbSw1;
  CButton cbSw2;
};
