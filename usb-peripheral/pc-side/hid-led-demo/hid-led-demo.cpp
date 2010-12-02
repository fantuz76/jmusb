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
#include "stdafx.h"
#include "hid-led-demo.h"
#include "hid-led-demoDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// ChidleddemoApp

BEGIN_MESSAGE_MAP(ChidleddemoApp, CWinApp)
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()


// ChidleddemoApp construction
ChidleddemoApp::ChidleddemoApp()
{
  //empty
}


// The one and only ChidleddemoApp object
ChidleddemoApp theApp;


// ChidleddemoApp initialization
BOOL ChidleddemoApp::InitInstance()
{
	// InitCommonControls() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	InitCommonControls();

	CWinApp::InitInstance();

	AfxEnableControlContainer();

  dlg=new(ChidleddemoDlg);
  if (dlg==NULL)
  {
    MessageBeep(MB_ICONHAND);
  }

  m_pMainWnd = dlg;
	INT_PTR nResponse = dlg->DoModal();
  delete(dlg);

  // Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
