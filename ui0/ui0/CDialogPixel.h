#pragma once
#include <d3d9.h>

// CDialogPixel 대화 상자

class CDialogPixel : public CDialogEx
{
	DECLARE_DYNAMIC(CDialogPixel)

public:
	CDialogPixel(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CDialogPixel();

	bool initView(int nID, RECT& rt, IDirect3DDevice9*& pDirect3DDeviceForPixel, IDirect3DSurface9*& pDirect3DSurfaceRenderForPixel);
	void render(unsigned char* buffer);
	void renderPixel(unsigned char* buffer, RECT& rt, IDirect3DDevice9* pDirect3DDeviceForPixel, IDirect3DSurface9* pDirect3DSurfaceRenderForPixel, D3DTEXTUREFILTERTYPE texFilter);

	IDirect3D9 *		m_pDirect3D9 = 0;
	char* m_partialWindow = 0;

	void DrawRectangle(IDirect3DDevice9* pDirect3DDeviceForPixel);

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_ANALYZE };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	virtual BOOL OnInitDialog();
	
};
