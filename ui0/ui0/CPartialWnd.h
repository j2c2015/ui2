#pragma once

// CPartialWnd

#define MAX_ZOOM_RATIO		(10)

class CConfigDialog;

class CPartialWnd : public CWnd
{
	DECLARE_DYNAMIC(CPartialWnd)

public:
	CPartialWnd();
	virtual ~CPartialWnd();

	CRect	m_rtWnd;
	int		m_nWndWidth;
	int		m_nWndHeight;
	CConfigDialog* m_pWndParent;

	BOOL	m_bDown;
	BOOL	m_bTracking;
	CPoint	m_ptDownPos;	
	CRect	m_rtRenderPosition;
	BOOL	m_bPositionNew;
	float	m_fBlockRenderWidth;
	float	m_fBlockRenderHeight;
	HBITMAP m_hbmpSave;

	CPoint	m_ptOriginPos;
	int		m_nCurPosX;
	int		m_nCurPosY;
	BYTE	m_bCurValue;
	int		m_nZoomRatio;

	void UpdatePartialView(TCHAR* szImgPath);
	BOOL CheckPositionDirty(CPoint point, CRect& rtNewPos);
	void GetPartialValue();

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
};


