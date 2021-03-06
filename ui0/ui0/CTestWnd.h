#pragma once

#include "CFileDropTarget.h"

// CTestWnd

class CConfigDialog;

class CTestWnd : public CWnd
{
	DECLARE_DYNAMIC(CTestWnd)

public:
	CTestWnd();
	virtual ~CTestWnd();

	CRect	m_rtWnd;
	int		m_nWndWidth;
	int		m_nWndHeight;
	CConfigDialog* m_pWndParentDlg;

	BOOL	m_bDown;
	BOOL	m_bTracking;
	CPoint	m_ptDownPos;
	CRect	m_rtPartial;
	CRect	m_rtRenderPartial;
	int		m_nSrcWidth;
	int		m_nSrcHeight;
	int		m_nPartialWidth;
	int		m_nPartialHeight;
	int		m_nBlockIndex;
	float	m_fBlockRenderWidth;
	float	m_fBlockRenderHeight;
	double	m_fRatioX;
	double	m_fRatioY;
	double	m_fRatioRenderX;
	double	m_fRatioRenderY;
	HBITMAP m_hbmpSave;

	void	CallPartailRender();
	Bitmap* m_pBmpImage;
	Bitmap* GetBmpImage() { return m_pBmpImage; }
	int		m_nBmpImgWidth;
	int		m_nBmpImgHeight;
	void	UpdateImage(Bitmap* pBmpImage);
	void	RecalcDimension();
	CRect	GetPartialRect() { return m_rtPartial; }
	CRect	GetRenderPartialRect() { return m_rtRenderPartial; }

	CFileDropTarget  m_FileDropTarget;
	
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg LRESULT OnUmFileDrop(WPARAM wParam, LPARAM lParam);	
};


