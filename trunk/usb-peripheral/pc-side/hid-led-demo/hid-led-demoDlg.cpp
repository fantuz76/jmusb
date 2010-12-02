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
// hid-led-demoDlg.cpp : implementation file
//
#include "stdafx.h"
#include "hid-led-demo.h"
#include "hid-led-demoDlg.h"
#include ".\hid-led-demodlg.h"
#include "hid_dev.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// ChidleddemoDlg dialog

static void update_sws(void);
static void update_leds(void);

//This timer callback procedure will be called periodically.
//It will update sw1 and sw2 state.
VOID CALLBACK my_t_proc(HWND hwnd, UINT uMsg,  UINT_PTR idEvent, DWORD dwTime)
{
  update_sws();
}

ChidleddemoDlg::ChidleddemoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(ChidleddemoDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void ChidleddemoDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_CHECK1, cbLed1);
  DDX_Control(pDX, IDC_CHECK2, cbLed2);
  DDX_Control(pDX, IDC_CHECK3, cbLed3);
  DDX_Control(pDX, IDC_CHECK4, cbLed4);
  DDX_Control(pDX, IDC_CHECK5, cbSw1);
  DDX_Control(pDX, IDC_CHECK6, cbSw2);
}

BEGIN_MESSAGE_MAP(ChidleddemoDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
  ON_BN_CLICKED(IDC_CHECK1, OnBnClickedCheck1)
  ON_BN_CLICKED(IDC_CHECK2, OnBnClickedCheck2)
  ON_BN_CLICKED(IDC_CHECK3, OnBnClickedCheck3)
  ON_BN_CLICKED(IDC_CHECK4, OnBnClickedCheck4)
END_MESSAGE_MAP()

// ChidleddemoDlg message handlers
BOOL ChidleddemoDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

  //Initialize HID module.
	if (HIDOpen())
  {
    MessageBox("HID initialization failed.", "Error", MB_OK);
    exit(1);
  };

  //Turn off all LEDs
  unsigned char c=0;
  HIDWrite(&c);

  timer=SetTimer(1, 10, my_t_proc);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void ChidleddemoDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
  CDialog::OnSysCommand(nID, lParam);
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.
void ChidleddemoDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR ChidleddemoDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

//This function will update LEDS on the board to reflect the state
//of GUI controls.
static void update_leds(void)
{
  unsigned char lstate=0;
  if (theApp.dlg->cbLed1.GetCheck())
  {
    lstate |= 1;
  }
  if (theApp.dlg->cbLed2.GetCheck())
  {
    lstate |= 2;
  }
  if (theApp.dlg->cbLed3.GetCheck())
  {
    lstate |= 4;
  }
  if (theApp.dlg->cbLed4.GetCheck())
  {
    lstate |= 8;
  }
  HIDWrite(&lstate);
}

//This function will update GUI control state to reflect the state 
//of sw1 nad sw2 on the board.
static void update_sws(void)
{
  unsigned char lstate=0;
  if (HIDRead(&lstate))
  {
    if (lstate & 0x1)
    {
      theApp.dlg->cbSw1.SetCheck(BST_CHECKED);
    }
    else
    {
      theApp.dlg->cbSw1.SetCheck(BST_UNCHECKED);
    }
    if (lstate & 0x2)
    {
      theApp.dlg->cbSw2.SetCheck(BST_CHECKED);
    }
    else
    {
      theApp.dlg->cbSw2.SetCheck(BST_UNCHECKED);
    }
  }
}

//Called when LED1 is clicked.
void ChidleddemoDlg::OnBnClickedCheck1()
{
  update_leds();
}

//Called when LED2 is clicked.
void ChidleddemoDlg::OnBnClickedCheck2()
{
  update_leds();
}

//Called when LED3 is clicked.
void ChidleddemoDlg::OnBnClickedCheck3()
{
  update_leds();
}

//Called when LED4 is clicked.
void ChidleddemoDlg::OnBnClickedCheck4()
{
  update_leds();
}
