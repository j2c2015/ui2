#pragma once

class CConfigDialog;
class CPartialWnd;

// CPartialZoomDlg dialog

class CPartialZoomDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CPartialZoomDlg)

public:
	CPartialZoomDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CPartialZoomDlg();

	CRect	m_rtDlg;
	int		m_nDlgWidth;
	int		m_nDlgHeight;
	float	m_fBlockRenderWidth;
	float	m_fBlockRenderHeight;

	CConfigDialog* m_pWndConfigDlg;
	CPartialWnd* m_pWndPartial;
	void SetParentWndDlg(CPartialWnd* pWndPartial, CConfigDialog* pWndParentDlg);
	BOOL	m_bDownCurView;	
	BOOL	m_bTracking;
	CPoint	m_ptDownPos;
	CRect	m_rtCurViewPosition;
	void UpdateCurViewPosition();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PARTIAL_ZOOM };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	virtual BOOL OnInitDialog();
};
