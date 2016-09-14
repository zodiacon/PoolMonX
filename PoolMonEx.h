
// PoolMonEx.h : main header file for the PoolMonEx application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols


// CPoolMonExApp:
// See PoolMonEx.cpp for the implementation of this class
//

class CPoolMonExApp : public CWinApp
{
public:
	CPoolMonExApp();


// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// Implementation

public:
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CPoolMonExApp theApp;
