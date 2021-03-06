
// ui0.h: PROJECT_NAME 응용 프로그램에 대한 주 헤더 파일입니다.
//

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH에 대해 이 파일을 포함하기 전에 'stdafx.h'를 포함합니다."
#endif

#include "resource.h"		// 주 기호입니다.

//
// GDI+
//
#include <GdiPlus.h>
#pragma comment(lib, "GdiPlus.lib")
using namespace Gdiplus;

#define UM_INIT_FILEADD			(WM_USER + 101)
#define UM_FILE_DROP			(WM_USER + 102)
#define UM_FILE_EYEFIND_TEST	(WM_USER + 103)

// Cui0App:
// 이 클래스의 구현에 대해서는 ui0.cpp을(를) 참조하세요.
//

class Cui0Dlg;

class Cui0App : public CWinApp
{
public:
	Cui0App();

	TCHAR m_szAppPath[_MAX_PATH];
	TCHAR m_szAppName[_MAX_PATH];
	TCHAR m_szConfPath[_MAX_PATH];
	TCHAR* GetAppConfPath() { return m_szConfPath; }

	ULONG_PTR			m_gdiplusToken;

// 재정의입니다.
public:
	virtual BOOL InitInstance();

// 구현입니다.

	DECLARE_MESSAGE_MAP()
	virtual int ExitInstance();
};

void DrawTextToDest(Graphics* pGraph,
	CString strText, CRect* pRtPS, CRect* pRtText,
	WCHAR* wszFontName, REAL nFontHeight,
	Gdiplus::StringAlignment alignment, Gdiplus::FontStyle style,
	Color crPen, StringFormatFlags strFormatFlags = StringFormatFlags(0),
	Gdiplus::StringAlignment lineAlignment = StringAlignmentNear);

extern Cui0App theApp;
extern Cui0Dlg* theMainDlg;
