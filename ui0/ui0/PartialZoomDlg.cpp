// PartialZoomDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ui0.h"
#include "ui0Dlg.h"
#include "PartialZoomDlg.h"
#include "afxdialogex.h"
#include "CPartialWnd.h"
#include "CConfigDialog.h"

// CPartialZoomDlg dialog

IMPLEMENT_DYNAMIC(CPartialZoomDlg, CDialogEx)

CPartialZoomDlg::CPartialZoomDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_PARTIAL_ZOOM, pParent)
{
	m_bDownCurView = FALSE;
	m_bTracking = FALSE;
	m_ptDownPos.x = m_ptDownPos.y = -1;
	m_pWndConfigDlg = NULL;
	m_pWndPartial = NULL;
}

CPartialZoomDlg::~CPartialZoomDlg()
{
}

void CPartialZoomDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

void CPartialZoomDlg::SetParentWndDlg(CPartialWnd* pWndPartial, CConfigDialog* pWndParentDlg)
{
	m_pWndPartial = pWndPartial;
	m_pWndConfigDlg = pWndParentDlg;
}

void CPartialZoomDlg::UpdateCurViewPosition()
{
	CRect rtPartialAllView = m_pWndPartial->GetAllViewPosition();
	CRect rtPartialCurView = m_pWndPartial->GetCurViewPosition();
	if (rtPartialAllView == rtPartialCurView)
	{
		m_rtCurViewPosition.SetRect(0, 0, m_nDlgWidth-1, m_nDlgHeight-1);
	}
	else
	{
		int nViewWidth = rtPartialCurView.Width() * m_nDlgWidth / rtPartialAllView.Width();
		int nViewHeight = rtPartialCurView.Height() * m_nDlgHeight / rtPartialAllView.Height();
		m_rtCurViewPosition.left = rtPartialCurView.left * m_nDlgWidth / rtPartialAllView.Width();
		m_rtCurViewPosition.top = rtPartialCurView.top * m_nDlgHeight / rtPartialAllView.Height();
		m_rtCurViewPosition.right = m_rtCurViewPosition.left + nViewWidth;
		m_rtCurViewPosition.bottom = m_rtCurViewPosition.top + nViewHeight;
	}
	Invalidate();
}

BEGIN_MESSAGE_MAP(CPartialZoomDlg, CDialogEx)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()


// CPartialZoomDlg message handlers

BOOL CPartialZoomDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_pWndPartial = (CPartialWnd*)GetParent();

	CRect rtAdjust(0, 0, 320, 320);
	AdjustWindowRect(&rtAdjust, WS_OVERLAPPEDWINDOW, FALSE);
	int nWidth = rtAdjust.right - rtAdjust.left;
	int nHeight = rtAdjust.bottom - rtAdjust.top;
	SetWindowPos(NULL, 0, 0, nWidth, nHeight, 0);

	GetClientRect(&m_rtDlg);
	m_nDlgWidth = m_rtDlg.Width();
	m_nDlgHeight = m_rtDlg.Height();

	int nPartialWidth = theMainDlg->GetPartialWidth();
	int nPartialHeight = theMainDlg->GetPartialHeight();
	m_fBlockRenderWidth = ((double)m_nDlgWidth / (double)nPartialWidth);
	m_fBlockRenderHeight = ((double)m_nDlgHeight / (double)nPartialHeight);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPartialZoomDlg::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}


void CPartialZoomDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
					   // TODO: Add your message handler code here
					   // Do not call CDialogEx::OnPaint() for painting messages

	if (!m_pWndPartial || !m_pWndConfigDlg)
		return;

	bool bRealView = false;
	if (m_pWndConfigDlg->GetViewMode() == CConfigDialog::eVIEW_REAL)
		bRealView = true;

	if (bRealView)
		return;

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

	Graphics graph(hdcMemory);

	if (hbmpMem)
	{
		graph.SetSmoothingMode(SmoothingModeAntiAlias);
		graph.SetInterpolationMode(InterpolationModeLowQuality);
		graph.SetPageUnit(UnitPixel);

		ImageAttributes ImgAttr;

		Bitmap* pBmpFixCrop = m_pWndConfigDlg->GetFixCropBmp();
		if (pBmpFixCrop)
		{
			{
				CRect rtCropPartial = m_pWndConfigDlg->GetPartialRect();
				int nISX = 0, nISY = 0, nICX = 0, nICY = 0;
				int nDSX = 0, nDSY = 0, nDCX = 0, nDCY = 0;
				nDSX = 0 - rtPS.left;
				nDSY = 0 - rtPS.top;
				nDCX = m_nDlgWidth;
				nDCY = m_nDlgHeight;
				RectF rf((REAL)nDSX, (REAL)nDSY, (REAL)nDCX, (REAL)nDCY);
				nISX = rtCropPartial.left;
				nISY = rtCropPartial.top;
				nICX = rtCropPartial.Width();
				nICY = rtCropPartial.Height();
				graph.DrawImage(pBmpFixCrop, rf, (REAL)nISX, (REAL)nISY, (REAL)nICX, (REAL)nICY, UnitPixel, &ImgAttr);
			}

			/*
			int nBmpWidth = pBmpFixCrop->GetWidth();
			int nBmpHeight = pBmpFixCrop->GetHeight();
			BitmapData bmpData;
			pBmpFixCrop->LockBits(new Rect(0, 0, nBmpWidth, nBmpHeight), ImageLockModeRead, PixelFormat32bppARGB, &bmpData);

			int stride = bmpData.Stride;
			UINT* pixels = (UINT*)bmpData.Scan0;

			CRect rtCropPartial = m_pWndConfigDlg->GetPartialRect();

			int nOriginX = 0;
			int nOriginY = 0;

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
					if ((x + w < 0 || x > m_nDlgWidth) || (y + h < 0 || y > m_nDlgHeight))
					{
						x += w;
						continue;
					}
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

					x += w;
				}
				y += h;
				x = nOriginX;
			}
			pBmpFixCrop->UnlockBits(&bmpData);
			*/

			//
			// current view
			//
			Color crFill = Color(64, 255, 128, 255);
			Color crPen = Color(255, 255, 0, 128);
			SolidBrush brushBG(crFill);
			Pen penBG(crPen, 1);
			REAL fSX = m_rtCurViewPosition.left - rtPS.left;
			REAL fSY = m_rtCurViewPosition.top - rtPS.top;
			REAL fWidth = m_rtCurViewPosition.Width();
			REAL fHeight = m_rtCurViewPosition.Height();
			graph.DrawRectangle(&penBG, fSX, fSY, fWidth, fHeight);
		}
	}
	BitBlt(dc.GetSafeHdc(), nPX, nPY, psWidth, psHeight, hdcMemory, 0, 0, SRCCOPY);

	::SelectObject(hdcMemory, hbmpOld);
	::DeleteObject(hbmpMem);

	::DeleteDC(hdcMemory);
}


void CPartialZoomDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetCapture();

	if (m_rtCurViewPosition.PtInRect(point))
	{
		m_bDownCurView = TRUE;
		m_ptDownPos = point;
	}

	CDialogEx::OnLButtonDown(nFlags, point);
}


void CPartialZoomDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_bDownCurView)
	{
		if ((point.x >= 0 && point.x <= m_nDlgWidth) && (point.y >= 0 && point.y <= m_nDlgHeight))
		{
			CPoint ptMove = point - m_ptDownPos;
			CRect rtCurView = m_rtCurViewPosition;

			rtCurView.OffsetRect(ptMove.x, ptMove.y);

			if ((rtCurView.left >= 0 && rtCurView.right <= m_nDlgWidth) &&
				(rtCurView.top >= 0 && rtCurView.bottom <= m_nDlgHeight))
			{
				m_rtCurViewPosition = rtCurView;
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

	CDialogEx::OnMouseMove(nFlags, point);
}


void CPartialZoomDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();

	if (m_bDownCurView)
	{
		m_pWndPartial->NotifyCurViewPosition(m_rtDlg, m_rtCurViewPosition);
	}
	m_bDownCurView = FALSE;

	CDialogEx::OnLButtonUp(nFlags, point);
}
