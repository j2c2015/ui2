// CTestWnd.cpp : implementation file
//

#include "stdafx.h"
#include "ui0.h"
#include "ui0Dlg.h"
#include "CConfigDialog.h"

//
// GDI+
//
#include <GdiPlus.h>
#pragma comment(lib, "GdiPlus.lib")
using namespace Gdiplus;

// CTestWnd

IMPLEMENT_DYNAMIC(CTestWnd, CWnd)

extern "C" void Test_partialRenderCb(void *buffer, int width, int height, int depth)
{
	pDlg->setPartialWindow((char *)buffer);

	// cscho (2018-12.11)
	//pDlg->m_pixelDialog.render((unsigned char*)buffer);
}

CTestWnd::CTestWnd()
{
	m_pWndParentDlg = NULL;
	m_bDown = FALSE;
	m_bTracking = FALSE;
	m_ptDownPos.x = m_ptDownPos.y = -1;
	m_rtPartial.SetRectEmpty();
	m_nSrcWidth = 800;
	m_nSrcHeight = 600;
	m_nPartialWidth = 64;
	m_nPartialHeight = 64;
	m_nBlockIndex = -1;
	m_hbmpSave = NULL;
	m_pBmpImage = NULL;
}

CTestWnd::~CTestWnd()
{
	if (m_hbmpSave)
	{
		::DeleteObject(m_hbmpSave);
		m_hbmpSave = NULL;
	}
}


void CTestWnd::CallPartailRender()
{
	partialRender(m_rtPartial.left, m_rtPartial.top, Test_partialRenderCb);
}

void CTestWnd::UpdateImage(Bitmap* pBmpImage)
{
	m_pBmpImage = pBmpImage;
	if (m_pBmpImage)
	{
		m_nSrcWidth = m_pBmpImage->GetWidth();
		m_nSrcHeight = m_pBmpImage->GetHeight();

		RecalcDimension();

		Invalidate();
	}
}

void CTestWnd::RecalcDimension()
{
	GetClientRect(&m_rtWnd);
	m_nWndWidth = m_rtWnd.Width();
	m_nWndHeight = m_rtWnd.Height();

	m_fRatioX = (double)m_nSrcWidth / (double)m_nWndWidth;
	m_fRatioY = (double)m_nSrcHeight / (double)m_nWndHeight;

	m_fRatioRenderX = (double)m_nWndWidth / (double)m_nSrcWidth;
	m_fRatioRenderY = (double)m_nWndHeight / (double)m_nSrcHeight;

	m_fBlockRenderWidth = (double)(m_nPartialWidth * m_nWndWidth) / (double)m_nSrcWidth;
	m_fBlockRenderHeight = (double)(m_nPartialHeight * m_nWndHeight) / (double)m_nSrcHeight;

	m_rtRenderPartial.right = m_rtRenderPartial.left + m_fBlockRenderWidth;
	m_rtRenderPartial.bottom = m_rtRenderPartial.top + m_fBlockRenderHeight;

	if (m_rtPartial.IsRectEmpty())
	{
		m_rtPartial.left = 0;
		m_rtPartial.top = 0;
		m_rtPartial.right = m_rtPartial.left + m_nPartialWidth;
		m_rtPartial.bottom = m_rtPartial.top + m_nPartialHeight;
	}
}

BEGIN_MESSAGE_MAP(CTestWnd, CWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_MESSAGE(UM_FILE_DROP, OnUmFileDrop)
END_MESSAGE_MAP()



// CTestWnd message handlers




int CTestWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_pWndParentDlg = (CConfigDialog*)GetParent();

	BOOL bReg = m_FileDropTarget.Register(this);

	return 0;
}


void CTestWnd::OnDestroy()
{
	m_FileDropTarget.Revoke();

	CWnd::OnDestroy();

	// TODO: Add your message handler code here
}


BOOL CTestWnd::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}


void CTestWnd::OnPaint()
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

	bool bRealView = false;
	if (m_pWndParentDlg->GetViewMode() == CConfigDialog::eVIEW_REAL)
		bRealView = true;

	HGDIOBJ hbmpOld = ::SelectObject(hdcMemory, hbmpMem);

	switch (m_pWndParentDlg->GetViewMode())
	{
	case CConfigDialog::eVIEW_REAL:
		{
			BitBlt(hdcMemory, 0, 0, nWndWidth, nWndHeight, hdc, 0, 0, SRCCOPY);
			m_hbmpSave = hbmpMem;
		}
		break;
	case CConfigDialog::eVIEW_IMAGE:
		{
			//
		}
		break;
	}
	
	Graphics graph(hdcMemory);

	WCHAR wszText[MAX_PATH] = L"";
	if (hbmpMem)
	{
		graph.SetSmoothingMode(SmoothingModeAntiAlias);
		graph.SetInterpolationMode(InterpolationModeHighQualityBicubic);
		graph.SetPageUnit(UnitPixel);

		ImageAttributes ImgAttr;

		switch (m_pWndParentDlg->GetViewMode())
		{
		case CConfigDialog::eVIEW_READY:
			{
				int nSX = 0 - rtPS.left;
				int nSY = 0 - rtPS.top;
				Color crFill = Color(255, 192, 192, 192);
				SolidBrush brushBG(crFill);
				graph.FillRectangle(&brushBG, nSX, nSY, nWndWidth - 1, nWndHeight - 1);
			}
			break;
		case CConfigDialog::eVIEW_REAL:
			{
				//
			}
			break;
		case CConfigDialog::eVIEW_IMAGE:
			{
				Bitmap* pBmpFixCrop = m_pWndParentDlg->GetFixCropBmp();
				if (pBmpFixCrop)
				{
					int nISX = 0, nISY = 0, nICX = 0, nICY = 0;
					int nDSX = 0, nDSY = 0, nDCX = 0, nDCY = 0;
					nDSX = 0 - rtPS.left;
					nDSY = 0 - rtPS.top;
					nDCX = nWndWidth;
					nDCY = nWndHeight;
					RectF rf((REAL)nDSX, (REAL)nDSY, (REAL)nDCX, (REAL)nDCY);
					nISX = 0;
					nISY = 0;
					nICX = pBmpFixCrop->GetWidth();
					nICY = pBmpFixCrop->GetHeight();
					graph.DrawImage(pBmpFixCrop, rf, (REAL)nISX, (REAL)nISY, (REAL)nICX, (REAL)nICY, UnitPixel, &ImgAttr);
				}
			}
			break;
		}

		// border
		int nSX = 0 - rtPS.left;
		int nSY = 0 - rtPS.top;
		Pen penBorder(Color(255, 128, 128, 128), 1);
		graph.DrawRectangle(&penBorder, nSX, nSY, nWndWidth - 1, nWndHeight - 1);

		REAL fCX, fCY, fSX, fSY, fRadius;

		// partial rectangle
		switch (m_pWndParentDlg->GetViewMode())
		{
		case CConfigDialog::eVIEW_REAL:
		case CConfigDialog::eVIEW_IMAGE:
			{
				int nItemWidth = 64, nItemHeight = 64;
				Color crFill = Color(64, 0, 255, 255);
				Color crPen = Color(255, 0, 128, 255);
				SolidBrush brushBG(crFill);
				Pen penBG(crPen, 1);
				fSX = m_rtRenderPartial.left - rtPS.left;
				fSY = m_rtRenderPartial.top - rtPS.top;
				graph.FillRectangle(&brushBG, fSX, fSY, m_fBlockRenderWidth, m_fBlockRenderHeight);
				graph.DrawRectangle(&penBG, fSX, fSY, m_fBlockRenderWidth, m_fBlockRenderHeight);
			}
			break;
		}

		// center circle
		if (m_pWndParentDlg->GetViewMode() == CConfigDialog::eVIEW_REAL)
		{			
			fCX = (nWndWidth / 2) - rtPS.left;
			fCY = (nWndHeight / 2) - rtPS.top;
			fRadius = 150.0f * m_fRatioRenderX;
			fSX = fCX - fRadius;
			fSY = fCY - fRadius;
			Pen penCenter(Color(255, 255, 255, 255), 1.5f);
			graph.DrawEllipse(&penCenter, fSX, fSY, fRadius * 2, fRadius * 2);
		}
	}

	BitBlt(dc.GetSafeHdc(), nPX, nPY, psWidth, psHeight, hdcMemory, 0, 0, SRCCOPY);

	::SelectObject(hdcMemory, hbmpOld);
	::DeleteObject(hbmpMem);

	::DeleteDC(hdcMemory);
}


void CTestWnd::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	RecalcDimension();
	
}


void CTestWnd::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_bDown)
	{
		if ((point.x >= 0 && point.x <= m_nWndWidth) && (point.y >= 0 && point.y <= m_nWndHeight))
		{
			CPoint ptMove = point - m_ptDownPos;
			CRect rtRenderPartial = m_rtRenderPartial;

			rtRenderPartial.left += ptMove.x;
			rtRenderPartial.top += ptMove.y;
			rtRenderPartial.right = rtRenderPartial.left + m_fBlockRenderWidth;
			rtRenderPartial.bottom = rtRenderPartial.top + m_fBlockRenderHeight;

			if ((rtRenderPartial.left >= 0 && rtRenderPartial.right <= m_nWndWidth) &&
				(rtRenderPartial.top >= 0 && rtRenderPartial.bottom <= m_nWndHeight))
			{
				m_rtRenderPartial = rtRenderPartial;
				Invalidate();
			}

			m_ptDownPos = point;
		}
	}

	if (!m_bTracking)
	{
		m_bTracking = TRUE;

		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(TRACKMOUSEEVENT);
		tme.dwFlags = TME_HOVER | TME_LEAVE;
		tme.dwHoverTime = 1;
		tme.hwndTrack = GetSafeHwnd();
		TrackMouseEvent(&tme);
	}

	CWnd::OnMouseMove(nFlags, point);
}


void CTestWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetCapture();
	
	switch (m_pWndParentDlg->GetViewMode())
	{
	case CConfigDialog::eVIEW_IMAGE:
	case CConfigDialog::eVIEW_REAL:
		{
			if (m_rtRenderPartial.PtInRect(point))
			{
				m_bDown = TRUE;
				m_ptDownPos = point;
			}
		}
		break;
	}
	
	CWnd::OnLButtonDown(nFlags, point);
}


void CTestWnd::OnLButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	SetFocus();

	m_bDown = FALSE;

	m_rtPartial.left = (int)(m_fRatioX * m_rtRenderPartial.left);
	m_rtPartial.top = (int)(m_fRatioY * m_rtRenderPartial.top);
	m_rtPartial.right = m_rtPartial.left + m_nPartialWidth;
	m_rtPartial.bottom = m_rtPartial.top + m_nPartialHeight;

	switch (m_pWndParentDlg->GetViewMode())
	{
	case CConfigDialog::eVIEW_REAL:
	case CConfigDialog::eVIEW_IMAGE:
		{
			m_pWndParentDlg->m_wndPartialView.UpdateView();
			//m_pWndParentDlg->m_wndPartialView.Invalidate();
		}
		break;
	}

	CWnd::OnLButtonUp(nFlags, point);
}

LRESULT CTestWnd::OnUmFileDrop(WPARAM wParam, LPARAM lParam)
{
	TCHAR* tszFilePath = (TCHAR*)lParam;
	if (!tszFilePath || _tcslen(tszFilePath) <= 0)
		return 0L;

	m_pWndParentDlg->UpdateAllView(tszFilePath);
}

