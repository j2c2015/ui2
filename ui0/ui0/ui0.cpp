
// ui0.cpp: 응용 프로그램에 대한 클래스 동작을 정의합니다.
//

#include "stdafx.h"
#include "ui0.h"
#include "ui0Dlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

void DrawTextToDest(Graphics* pGraph,
	CString strText, CRect* pRtPS, CRect* pRtText,
	WCHAR* wszFontName, REAL nFontHeight,
	Gdiplus::StringAlignment alignment, Gdiplus::FontStyle style,
	Color crPen, StringFormatFlags strFormatFlags,
	Gdiplus::StringAlignment lineAlignment)
{
	CRect rtIntersect;
	int nISX = 0, nISY = 0, nICX = 0, nICY = 0, nImgWidth = 0, nImgHeight = 0;
	int nDSX = 0, nDSY = 0, nDCX = 0, nDCY = 0;

	if ((strText.GetLength() > 0) && (rtIntersect.IntersectRect(pRtPS, pRtText)))
	{
		nDSX = pRtText->left - pRtPS->left;
		nDSY = pRtText->top - pRtPS->top;

		Gdiplus::FontFamily myFontFamily(wszFontName);
		Gdiplus::Status stFontFamily = myFontFamily.GetLastStatus();
		Gdiplus::Font* pMyFont = NULL;
		Gdiplus::Font myFont(wszFontName, nFontHeight, style);
		Gdiplus::Font myFontGeneric(Gdiplus::FontFamily::GenericSansSerif(), nFontHeight, style);
		if (stFontFamily == Gdiplus::Ok)
		{
			pMyFont = &myFont;
		}
		else
		{
			pMyFont = &myFontGeneric;
		}

		StringFormat format;
		format.SetAlignment(alignment);
		format.SetLineAlignment(lineAlignment);
		format.SetFormatFlags(strFormatFlags);
		format.SetTrimming(StringTrimmingEllipsisCharacter);
		
		SolidBrush brush(crPen);
		RectF layoutRect((Gdiplus::REAL)nDSX,
			(Gdiplus::REAL)nDSY,
			(Gdiplus::REAL)(pRtText->Width()),
			(Gdiplus::REAL)(pRtText->Height()));

		pGraph->DrawString(strText,
			strText.GetLength(),
			pMyFont,
			layoutRect,
			&format,
			&brush);
	}
}

// Cui0App

BEGIN_MESSAGE_MAP(Cui0App, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// Cui0App 생성

Cui0App::Cui0App()
{
	// 다시 시작 관리자 지원
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: 여기에 생성 코드를 추가합니다.
	// InitInstance에 모든 중요한 초기화 작업을 배치합니다.
}


// 유일한 Cui0App 개체입니다.

Cui0App theApp;
Cui0Dlg* theMainDlg;

// Cui0App 초기화

BOOL Cui0App::InitInstance()
{
	// 응용 프로그램 매니페스트가 ComCtl32.dll 버전 6 이상을 사용하여 비주얼 스타일을
	// 사용하도록 지정하는 경우, Windows XP 상에서 반드시 InitCommonControlsEx()가 필요합니다.
	// InitCommonControlsEx()를 사용하지 않으면 창을 만들 수 없습니다.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// 응용 프로그램에서 사용할 모든 공용 컨트롤 클래스를 포함하도록
	// 이 항목을 설정하십시오.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	if (!AfxOleInit())
	{
		//
		return FALSE;
	}

	CWinApp::InitInstance();
	
	RUN_MODULE_NAME(m_hInstance, m_szAppPath, m_szAppName, NULL);
	LOG_INIT(m_hInstance);
	J2CLogInit(m_hInstance);

	_stprintf(m_szConfPath, _T("%s%s_conf.ini"), m_szAppPath, m_szAppName);

	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);

	FUNCTION_NAME_IS(_T("Cui0App::InitInstance"));
	LOG_PUTS(0, _T("***** J2C sample starts. *****"));
	
	Cui0Dlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		//
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: 여기에 [취소]를 클릭하여 대화 상자가 없어질 때 처리할
		//  코드를 배치합니다.
	}
	else if (nResponse == -1)
	{
		TRACE(traceAppMsg, 0, "경고: 대화 상자를 만들지 못했으므로 응용 프로그램이 예기치 않게 종료됩니다.\n");
		TRACE(traceAppMsg, 0, "경고: 대화 상자에서 MFC 컨트롤을 사용하는 경우 #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS를 수행할 수 없습니다.\n");
	}
	
#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
	ControlBarCleanUp();
#endif

	// 대화 상자가 닫혔으므로 응용 프로그램의 메시지 펌프를 시작하지 않고  응용 프로그램을 끝낼 수 있도록 FALSE를
	// 반환합니다.
	return FALSE;
}



int Cui0App::ExitInstance()
{
	Gdiplus::GdiplusShutdown(m_gdiplusToken);

	FUNCTION_NAME_IS(_T("Cui0App::ExitInstance"));
	LOG_PUTS(0, _T("***** J2C sample exits. *****"));
	
	return CWinApp::ExitInstance();
}
