#pragma once

#include "PartialZoomDlg.h"

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
	CConfigDialog* m_pWndParentDlg;
	CConfigDialog* GetParentWndDlg() { return m_pWndParentDlg; }

	BOOL	m_bDown;
	BOOL	m_bTracking;
	CPoint	m_ptDownPos;
	CRect	m_rtRenderPosition;
	CRect	m_rtAllViewPosition;
	CRect	m_rtCurViewPosition;
	CRect	GetAllViewPosition() { return m_rtAllViewPosition; }
	CRect	GetCurViewPosition() { return m_rtCurViewPosition; }
	BOOL	m_bPositionNew;
	float	m_fBlockRenderWidth;
	float	m_fBlockRenderHeight;
	HBITMAP m_hbmpSave;

	CPoint	m_ptOriginPos;
	CPoint	GetOriginPos() { return m_ptOriginPos; }
	int		m_nCurPosX;
	int		m_nCurPosY;
	BYTE	m_bCurValue;
	int		m_nZoomRatio;
	int		GetZoomRatio() { return m_nZoomRatio; }

	bool	m_bCtrlDown;
	bool	m_bCtrlRegionValid;
	CRect	m_rtCtrlDownPos;
	CRect	m_rtCtrlAllPos;
	int		m_nCtrlPosSX;
	int		m_nCtrlPosSY;
	int		m_nCtrlPosEX;
	int		m_nCtrlPosEY;

	CPartialZoomDlg m_ZoomDlg;
	void UpdateView();
	void UpdateImage();
	void UpdateZoomDlg();
	void NotifyCurViewPosition(CRect rtAllViewPosition, CRect rtCurViewPosition);

	void UpdatePartialView(TCHAR* szImgPath);
	BOOL CheckPositionDirty(CPoint point, CRect& rtNewPos, int& nNewPosX, int& nNewPosY);
	BOOL GetCtrlPositionValue(CByteArray* pArrValue);
	void GetPartialValue();
	void GetPartialRange();
	void CopyValueToClipboard();
	void RecalcDimension();
	void UpdateCurViewPosition();

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
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};


