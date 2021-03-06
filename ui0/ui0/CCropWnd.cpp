// CCropWnd.cpp : implementation file
//

#include "stdafx.h"
#include "ui0.h"
#include "CCropWnd.h"

//
// GDI+
//
#include <GdiPlus.h>
#pragma comment(lib, "GdiPlus.lib")
using namespace Gdiplus;

// CCropWnd

IMPLEMENT_DYNAMIC(CCropWnd, CWnd)

CCropWnd::CCropWnd()
{

}

CCropWnd::~CCropWnd()
{
}

void CCropWnd::UpdateCropView(TCHAR* szImgPath)
{
	//
}

BEGIN_MESSAGE_MAP(CCropWnd, CWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()



// CCropWnd message handlers




int CCropWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  Add your specialized creation code here

	return 0;
}


void CCropWnd::OnDestroy()
{
	CWnd::OnDestroy();

	// TODO: Add your message handler code here
}


void CCropWnd::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
}


BOOL CCropWnd::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}


void CCropWnd::OnPaint()
{
	CPaintDC dc(this); // device context for painting
					   // TODO: Add your message handler code here
					   // Do not call CWnd::OnPaint() for painting messages

	PAINTSTRUCT ps = dc.m_ps;
	int psWidth = ps.rcPaint.right - ps.rcPaint.left;
	int psHeight = ps.rcPaint.bottom - ps.rcPaint.top;

	int nPX = ps.rcPaint.left;
	int nPY = ps.rcPaint.top;

	CRect rtPS(nPX, nPY, nPX + psWidth, nPY + psHeight);

	CRect rt;
	GetClientRect(&rt);
	int nWndWidth = rt.Width();
	int nWndHeight = rt.Height();

	HDC hdc = dc.GetSafeHdc();

	HDC	hdcMemory = CreateCompatibleDC(hdc);
	HBITMAP hbmpMem = CreateCompatibleBitmap(hdc, psWidth, psHeight);

	HGDIOBJ hbmpOld = ::SelectObject(hdcMemory, hbmpMem);
	BitBlt(hdcMemory, 0, 0, nWndWidth, nWndHeight, hdc, 0, 0, SRCCOPY);

	Graphics graph(hdcMemory);

	WCHAR wszText[MAX_PATH] = L"";
	if (hbmpMem)
	{
		graph.SetSmoothingMode(SmoothingModeAntiAlias);
		graph.SetInterpolationMode(InterpolationModeHighQualityBicubic);
		graph.SetPageUnit(UnitPixel);

		ImageAttributes ImgAttr;

		//
	}

	BitBlt(dc.GetSafeHdc(), nPX, nPY, psWidth, psHeight, hdcMemory, 0, 0, SRCCOPY);

	::SelectObject(hdcMemory, hbmpOld);
	::DeleteObject(hbmpMem);

	::DeleteDC(hdcMemory);
}


void CCropWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CWnd::OnLButtonDown(nFlags, point);
}


void CCropWnd::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CWnd::OnMouseMove(nFlags, point);
}


void CCropWnd::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CWnd::OnLButtonUp(nFlags, point);
}
