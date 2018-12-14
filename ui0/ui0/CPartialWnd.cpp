// CPartialWnd.cpp : implementation file
//

#include "stdafx.h"
#include "ui0.h"
#include "CPartialWnd.h"
#include "CConfigDialog.h"
#include "ui0Dlg.h"

//
// GDI+
//
#include <GdiPlus.h>
#pragma comment(lib, "GdiPlus.lib")
using namespace Gdiplus;

// CPartialWnd

IMPLEMENT_DYNAMIC(CPartialWnd, CWnd)

CPartialWnd::CPartialWnd()
{
	m_pWndParent = NULL;
	m_bDown = FALSE;
	m_bTracking = FALSE;
	m_ptDownPos.x = m_ptDownPos.y = -1;
	m_hbmpSave = NULL;
	m_nZoomRatio = 1;
	m_rtRenderPosition.SetRectEmpty();
	m_bPositionNew = FALSE;
	m_nCurPosX = -1;
	m_nCurPosY = -1;
	m_bCurValue = 0;
	m_ptOriginPos.x = m_ptOriginPos.y = 0;
}

CPartialWnd::~CPartialWnd()
{
	if (m_hbmpSave)
	{
		::DeleteObject(m_hbmpSave);
		m_hbmpSave = NULL;
	}
}

void CPartialWnd::UpdatePartialView(TCHAR* szImgPath)
{
	//
}

BOOL CPartialWnd::CheckPositionDirty(CPoint point, CRect& rtNewPos)
{
	BOOL bIsDirty = FALSE;

	int x = m_ptOriginPos.x;
	int y = m_ptOriginPos.y;
	int nPartialWidth = theMainDlg->GetPartialWidth();
	int nPartialHeight = theMainDlg->GetPartialHeight();
	int w = m_fBlockRenderWidth;
	int h = m_fBlockRenderHeight;

	BOOL bExist = FALSE;
	for (UINT row = 0; row < nPartialHeight; row++)
	{
		for (UINT col = 0; col < nPartialWidth; col++)
		{
			CRect rtTest(x, y, x + w, y + h);
			if (rtTest.PtInRect(point))
			{
				if (m_rtRenderPosition.EqualRect(&rtTest) == FALSE)
				{
					bIsDirty = TRUE;
					bExist = TRUE;
					rtNewPos = rtTest;
					m_nCurPosX = col;
					m_nCurPosY = row;
					break;
				}
			}
			x += w;
		}
		y += h;
		x = m_ptOriginPos.x;
	}

	return bIsDirty;
}

void CPartialWnd::GetPartialValue()
{
	CString strValue = _T("");

	UINT nValue = 0;
	Bitmap* pBmpFixCrop = m_pWndParent->GetFixCropBmp();
	if (pBmpFixCrop)
	{
		int nBmpWidth = pBmpFixCrop->GetWidth();
		int nBmpHeight = pBmpFixCrop->GetHeight();
		BitmapData bmpData;
		pBmpFixCrop->LockBits(new Rect(0, 0, nBmpWidth, nBmpHeight), ImageLockModeRead, PixelFormat32bppARGB, &bmpData);

		int stride = bmpData.Stride;
		UINT* pixels = (UINT*)bmpData.Scan0;

		CRect rtCropPartial = m_pWndParent->GetPartialRect();
		int nIndexCol = rtCropPartial.left + m_nCurPosX;
		int nIndexRow = rtCropPartial.top + m_nCurPosY;
		
		if (pixels)
		{
			UINT curColor = pixels[nIndexRow * stride / 4 + nIndexCol];
			UINT b = curColor & 0xff;
			UINT g = (curColor & 0xff00) >> 8;
			UINT r = (curColor & 0xff0000) >> 16;
			UINT a = (curColor & 0xff000000) >> 24;
			nValue = r;
		}
		
		pBmpFixCrop->UnlockBits(&bmpData);
	}
	strValue.Format(_T("Value: %d [ X: %d, Y: %d ]"), nValue, m_nCurPosX, m_nCurPosY);
	m_pWndParent->UpdatePartialValue(strValue);
	
	FUNCTION_NAME_IS(_T("CPartialWnd::GetPartialValue"));
	LOG_PRINTF(0, _T("Partial view clicked. %s"), strValue);
}

BEGIN_MESSAGE_MAP(CPartialWnd, CWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()



// CPartialWnd message handlers




int CPartialWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_pWndParent = (CConfigDialog*)GetParent();

	return 0;
}


void CPartialWnd::OnDestroy()
{
	CWnd::OnDestroy();

	// TODO: Add your message handler code here
}


void CPartialWnd::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	GetClientRect(&m_rtWnd);
	m_nWndWidth = m_rtWnd.Width();
	m_nWndHeight = m_rtWnd.Height();

	int nPartialWidth = theMainDlg->GetPartialWidth();
	int nPartialHeight = theMainDlg->GetPartialHeight();
	m_fBlockRenderWidth = (m_nWndWidth / nPartialWidth) * m_nZoomRatio;
	m_fBlockRenderHeight = (m_nWndHeight / nPartialHeight) * m_nZoomRatio;

}


BOOL CPartialWnd::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}


void CPartialWnd::OnPaint()
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
	if (m_pWndParent->GetViewMode() == CConfigDialog::eVIEW_REAL)
		bRealView = true;

	HGDIOBJ hbmpOld = ::SelectObject(hdcMemory, hbmpMem);

	switch (m_pWndParent->GetViewMode())
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
		graph.SetInterpolationMode(InterpolationModeLowQuality);
		graph.SetPageUnit(UnitPixel);

		ImageAttributes ImgAttr;

		int nOriginX = m_ptOriginPos.x;
		int nOriginY = m_ptOriginPos.y;
		switch (m_pWndParent->GetViewMode())
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
				Bitmap* pBmpFixCrop = m_pWndParent->GetFixCropBmp();
				if (pBmpFixCrop)
				{
					int nBmpWidth = pBmpFixCrop->GetWidth();
					int nBmpHeight = pBmpFixCrop->GetHeight();
					BitmapData bmpData;
					pBmpFixCrop->LockBits(new Rect(0, 0, nBmpWidth, nBmpHeight), ImageLockModeRead, PixelFormat32bppARGB, &bmpData);
										
					int stride = bmpData.Stride;
					UINT* pixels = (UINT*)bmpData.Scan0;

					CRect rtCropPartial = m_pWndParent->GetPartialRect();

					int x = nOriginX - rtPS.left;
					int y = nOriginY - rtPS.top;
					int nPartialWidth = theMainDlg->GetPartialWidth();
					int nPartialHeight = theMainDlg->GetPartialHeight();
					int w = m_fBlockRenderWidth;
					int h = m_fBlockRenderHeight;

					TCHAR szValue[10] = _T("");
					CRect rtText(0, 0, 0, 0);
					Color crText = Color::Gray;
					REAL fTextHeight = 9.0f;
					
					int nRowBegin = rtCropPartial.top;
					int nRowEnd = nRowBegin + nPartialWidth;
					int nColBegin = rtCropPartial.left;
					int nColEnd = nColBegin + nPartialHeight;
					//for (UINT row = 0; row < nBmpHeight; row++)
					for (UINT row = nRowBegin; row < nRowEnd; row++)
					{
						//for (UINT col = 0; col < nBmpWidth; col++)
						for (UINT col = nColBegin; col < nColEnd; col++)
						{
							unsigned int curColor = pixels[row * stride / 4 + col];
							int b = curColor & 0xff;
							int g = (curColor & 0xff00) >> 8;
							int r = (curColor & 0xff0000) >> 16;
							int a = (curColor & 0xff000000) >> 24;
						
							Color crFill = Color(255, r, g, b);
							SolidBrush brushBG(crFill);
							Pen penBG(crFill, 1);
							graph.FillRectangle(&brushBG, x, y, w, h);
							graph.DrawRectangle(&penBG, x, y, w, h);

							// test
							bool bShowValue = m_pWndParent->GetPartialShowValue();
							if (bShowValue && m_nZoomRatio > 1)
							{
								rtText.SetRect(x, y, x + w, y + h);
								if (r > 128)
									crText = Color::Black;
								else
									crText = Color::White;

								if (m_nZoomRatio == 2)
									fTextHeight = 5.5f;
								else if (m_nZoomRatio == 3)
									fTextHeight = 6.5f;
								else if (m_nZoomRatio < 6)
									fTextHeight = 7.5f;
								else
									fTextHeight = 8.5f;

								_stprintf(szValue, _T("%d"), r);
								DrawTextToDest(&graph,
									szValue, &rtPS, &rtText,
									_T("System"), fTextHeight,
									StringAlignmentCenter,
									FontStyleRegular,
									crText,
									StringFormatFlags(0),
									StringAlignmentCenter);
							}
							x += w;
						}
						y += h;
						x = nOriginX;
					}
					pBmpFixCrop->UnlockBits(&bmpData);
				}
			}
			break;
		}

		// border
		int nSX = 0 - rtPS.left;
		int nSY = 0 - rtPS.top;
		Pen penBorder(Color(255, 128, 128, 128), 1);
		graph.DrawRectangle(&penBorder, nSX, nSY, nWndWidth - 1, nWndHeight - 1);

		// position rect
		if (m_pWndParent->GetViewMode() == CConfigDialog::eVIEW_IMAGE && m_bPositionNew)
		{
			int nItemWidth = 64, nItemHeight = 64;
			Color crFill = Color(64, 255, 128, 255);
			Color crPen = Color(255, 255, 0, 128);
			SolidBrush brushBG(crFill);
			Pen penBG(crPen, 1);
			REAL fSX = m_rtRenderPosition.left - rtPS.left;
			REAL fSY = m_rtRenderPosition.top - rtPS.top;
			//graph.FillRectangle(&brushBG, fSX, fSY, m_fBlockRenderWidth, m_fBlockRenderHeight);
			graph.DrawRectangle(&penBG, fSX, fSY, m_fBlockRenderWidth, m_fBlockRenderHeight);
		}
	}

	BitBlt(dc.GetSafeHdc(), nPX, nPY, psWidth, psHeight, hdcMemory, 0, 0, SRCCOPY);

	::SelectObject(hdcMemory, hbmpOld);
	::DeleteObject(hbmpMem);

	::DeleteDC(hdcMemory);
}


void CPartialWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetCapture();

	m_bDown = TRUE;
	m_ptDownPos = point;

	CRect rtNewPos(0, 0, 0, 0);
	BOOL bIsDirty = CheckPositionDirty(point, rtNewPos);

	if (bIsDirty)
	{
		m_bPositionNew = TRUE;
		m_rtRenderPosition = rtNewPos;
		Invalidate();

		GetPartialValue();
	}

	CWnd::OnLButtonDown(nFlags, point);
}


void CPartialWnd::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_bDown)
	{
		if (m_nZoomRatio > 1)
		{
			if ((point.x >= 0 && point.x <= m_nWndWidth) && (point.y >= 0 && point.y <= m_nWndHeight))
			{
				CPoint ptMove = point - m_ptDownPos;
				CPoint ptOriginPos = m_ptOriginPos;
				ptOriginPos.x += ptMove.x;
				ptOriginPos.y += ptMove.y;

				int nPartialWidth = theMainDlg->GetPartialWidth();
				int nPartialHeight = theMainDlg->GetPartialHeight();
				int nEndPosX = ptOriginPos.x + (nPartialWidth * m_fBlockRenderWidth);
				int nEndPosY = ptOriginPos.y + (nPartialHeight * m_fBlockRenderHeight);

				if ((ptOriginPos.x <= 0 && ptOriginPos.y <= 0) &&
					(nEndPosX >= m_nWndWidth && nEndPosY >= m_nWndHeight))
				{
					m_rtRenderPosition.OffsetRect(ptMove.x, ptMove.y);
					m_ptOriginPos = ptOriginPos;
					
					Invalidate();
				}
				m_ptDownPos = point;
			}
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


void CPartialWnd::OnLButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	SetFocus();

	m_bDown = FALSE;

	CWnd::OnLButtonUp(nFlags, point);
}


BOOL CPartialWnd::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	switch (m_pWndParent->GetViewMode())
	{
	case CConfigDialog::eVIEW_REAL:
		{
			//
		}
		break;
	case CConfigDialog::eVIEW_IMAGE:
		{
			int nScrollSize = 0;
			//if (GetAsyncKeyState(VK_CONTROL) < 0)
			{
				int nZoomRatioOld = m_nZoomRatio;
				CPoint ptOriginOld = m_ptOriginPos;

				nScrollSize = zDelta / WHEEL_DELTA;
				m_nZoomRatio += nScrollSize;
				if (m_nZoomRatio < 1)
				{
					m_nZoomRatio = 1;
				}
				else if (m_nZoomRatio > MAX_ZOOM_RATIO)
				{
					m_nZoomRatio = MAX_ZOOM_RATIO;
				}
				else
				{
					//
				}

				if (m_nZoomRatio == 1)
				{
					m_ptOriginPos = CPoint(0, 0);
				}
				else
				{
					if (nZoomRatioOld > 1)
					{
						m_ptOriginPos.x = (ptOriginOld.x * (m_nZoomRatio - 1)) / (nZoomRatioOld - 1);
						m_ptOriginPos.y = (ptOriginOld.y * (m_nZoomRatio - 1)) / (nZoomRatioOld - 1);
					}
				}
				CRect rt;
				GetClientRect(&rt);
				int nWndWidth = rt.Width();
				int nWndHeight = rt.Height();

				int nPartialWidth = theMainDlg->GetPartialWidth();
				int nPartialHeight = theMainDlg->GetPartialHeight();
				m_fBlockRenderWidth = (nWndWidth / nPartialWidth) * m_nZoomRatio;
				m_fBlockRenderHeight = (nWndHeight / nPartialHeight) * m_nZoomRatio;

				m_bPositionNew = FALSE;
				Invalidate();
			}
		}
		break;
	}

	return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}
