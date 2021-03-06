// CDialogPixel.cpp: 구현 파일
//

#include "stdafx.h"
#include "ui0.h"
#include "CDialogPixel.h"
#include "afxdialogex.h"
#include "ui0Dlg.h"
#include <iostream>
#include "J2CEngine_dll.h"

CDialogPixel *pPixelDlg = 0;
// CDialogPixel 대화 상자

IMPLEMENT_DYNAMIC(CDialogPixel, CDialogEx)

CDialogPixel::CDialogPixel(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_ANALYZE, pParent)
{

}

CDialogPixel::~CDialogPixel()
{
	if (m_partialWindow)
	{
		free(m_partialWindow);
		m_partialWindow = 0;
	}
}

void CDialogPixel::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

bool CDialogPixel::initView(int nID, RECT& rt, IDirect3DDevice9*& pDirect3DDeviceForPixel, IDirect3DSurface9*& pDirect3DSurfaceRenderForPixel)
{
	HWND hwnd = GetDlgItem(nID)->m_hWnd;
	bool	bSuccess = false;
	HRESULT hrSuccess;

	D3DPRESENT_PARAMETERS d3dpp;
	D3DFORMAT Format;
	__try
	{
		ZeroMemory(&d3dpp, sizeof(d3dpp));
		d3dpp.Windowed = TRUE;
		d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

		::GetClientRect(hwnd, &rt);
		hrSuccess = m_pDirect3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDirect3DDeviceForPixel);
		if (FAILED(hrSuccess))
		{
			FUNCTION_NAME_IS(_T("CDialogPixel::initView"));
			LOG_PUTS(0, _T("Failed to create D3D device."));
			__leave;
		}

		Format = (D3DFORMAT)MAKEFOURCC('Y', 'V', '1', '2');	// J2C
		//Format = D3DFMT_A8R8G8B8;
		hrSuccess = pDirect3DDeviceForPixel->CreateOffscreenPlainSurface(pDlg->m_nPartialWidth, pDlg->m_nPartialHeight, Format, D3DPOOL_DEFAULT, &pDirect3DSurfaceRenderForPixel, NULL);
		if (FAILED(hrSuccess))
		{
			FUNCTION_NAME_IS(_T("CDialogPixel::initView"));
			LOG_PRINTF(0, _T("Failed to create D3D PlainSurface[1]. (Width=%d, Height=%d)"), pDlg->m_nPartialWidth, pDlg->m_nPartialHeight);
			__leave;
		}
		
		bSuccess = true;
	}
	__finally
	{
		if (false == bSuccess)
		{
			pDlg->cleanupForPixel();
		}
	}

	return bSuccess;
}

void CDialogPixel::render(unsigned char* pBuffer)
{
	D3DTEXTUREFILTERTYPE texFilterLin = D3DTEXF_LINEAR;
	D3DTEXTUREFILTERTYPE texFilterNon = D3DTEXF_NONE;
	renderPixel(pBuffer, pDlg->m_PixelRenderViewport, pDlg->m_pDirect3DDeviceForPixel, pDlg->m_pDirect3DSurfaceRenderForPixel, texFilterNon);
	renderPixel(pBuffer, pDlg->m_PixelRenderViewport2, pDlg->m_pDirect3DDeviceForPixel2, pDlg->m_pDirect3DSurfaceRenderForPixel2, texFilterNon);
}

void CDialogPixel::renderPixel(unsigned char* pBuffer, RECT& rt, IDirect3DDevice9* pDirect3DDeviceForPixel, IDirect3DSurface9* pDirect3DSurfaceRenderForPixel, D3DTEXTUREFILTERTYPE texFilter)
{
	HRESULT lRet;

	if (pDirect3DSurfaceRenderForPixel == NULL)
		return;

	D3DLOCKED_RECT d3d_rect;
	lRet = pDirect3DSurfaceRenderForPixel->LockRect(&d3d_rect, NULL, D3DLOCK_DONOTWAIT);
	if (FAILED(lRet))
		return;

	byte *pSrc = pBuffer;
	byte * pDest = (BYTE *)d3d_rect.pBits;
	int stride = d3d_rect.Pitch;
	unsigned long nIndex = 0;

	size_t size = (stride)*pDlg->m_nPartialHeight*BPP / 8;
	memset(pDest, 0x80, size);
	for (nIndex = 0; nIndex < pDlg->m_nPartialHeight; nIndex++)
	{
		memcpy(pDest + nIndex * stride, pSrc + nIndex * pDlg->m_nPartialWidth, pDlg->m_nPartialWidth);
	}

	lRet = pDirect3DSurfaceRenderForPixel->UnlockRect();
	if (FAILED(lRet))
		return;

	if (pDirect3DDeviceForPixel == NULL)
		return;

	pDirect3DDeviceForPixel->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(128, 128, 128), 1.0f, 0);
	pDirect3DDeviceForPixel->BeginScene();
	IDirect3DSurface9 * pBackBuffer = NULL;

	pDirect3DDeviceForPixel->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
	pDirect3DDeviceForPixel->StretchRect(pDirect3DSurfaceRenderForPixel, NULL, pBackBuffer, &rt, texFilter);

	//DrawRectangle(pDirect3DDeviceForPixel);
		
	pDirect3DDeviceForPixel->EndScene();
	pDirect3DDeviceForPixel->Present(NULL, NULL, NULL, NULL);
}

void CDialogPixel::DrawRectangle(IDirect3DDevice9* pDirect3DDeviceForPixel)
{
	pDirect3DDeviceForPixel->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);

	pDirect3DDeviceForPixel->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	pDirect3DDeviceForPixel->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
	pDirect3DDeviceForPixel->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

	int x = 0;
	int y = 0;
	int w = 50;
	int h = 50;
	D3DCOLOR color1 = D3DCOLOR_ARGB(16, 255, 255, 0);
	D3DCOLOR color2 = D3DCOLOR_ARGB(16, 255, 255, 0);
	D3DCOLOR color3 = D3DCOLOR_ARGB(16, 255, 255, 0);
	D3DCOLOR color4 = D3DCOLOR_ARGB(16, 255, 255, 0);

	struct
	{
		float x, y, z, w;
		D3DCOLOR color;
	} vert[4] = {
		x - 0.5f, y - 0.5f, 1.f, 1.f, color1,
		x + w - 0.5f, y - 0.5f, 1.f, 1.f, color2,
		x - 0.5f, y + h - 0.5f, 1.f, 1.f, color3,
		x + w - 0.5f, y + h - 0.5f, 1.f, 1.f, color4
	};
	HRESULT hr = pDirect3DDeviceForPixel->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vert, 20);
}

BEGIN_MESSAGE_MAP(CDialogPixel, CDialogEx)
	ON_WM_PAINT()
	
END_MESSAGE_MAP()


// CDialogPixel 메시지 처리기

BOOL CDialogPixel::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	pPixelDlg = this;

	m_partialWindow = (char*)calloc(sizeof(char), pDlg->m_nPartialWidth*pDlg->m_nPartialHeight);

	m_pDirect3D9 = Direct3DCreate9(D3D_SDK_VERSION);

	bool bRet = initView(IDC_PIXEL_RENDER_VIEW, pDlg->m_PixelRenderViewport, pDlg->m_pDirect3DDeviceForPixel, pDlg->m_pDirect3DSurfaceRenderForPixel);
	if (bRet == false)
	{
		LOG_PUTS(0, _T("Failed to init pixel view.[IDC_PIXEL_RENDER_VIEW]"));
	}
	bool bRet2 = initView(IDC_PIXEL_RENDER_VIEW2, pDlg->m_PixelRenderViewport2, pDlg->m_pDirect3DDeviceForPixel2, pDlg->m_pDirect3DSurfaceRenderForPixel2);
	if (bRet2 == false)
	{
		LOG_PUTS(0, _T("Failed to init pixel view.[IDC_PIXEL_RENDER_VIEW2]"));
	}

	CWnd* pWndView = GetDlgItem(IDC_PIXEL_RENDER_VIEW);
	if (pWndView)
	{
		int nViewWidth = 512, nViewHeight = 512;
		pWndView->SetWindowPos(NULL, 0, 0, nViewWidth, nViewHeight, SWP_NOMOVE | SWP_NOZORDER);
	}
	CWnd* pWndView2 = GetDlgItem(IDC_PIXEL_RENDER_VIEW2);
	if (pWndView2)
	{
		if (pWndView)
		{
			//
		}
		pWndView2->SetWindowPos(NULL, 0, 0, pDlg->m_nPartialWidth, pDlg->m_nPartialHeight, SWP_NOMOVE | SWP_NOZORDER);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

void CDialogPixel::OnPaint()
{
	CPaintDC dc(this); // device context for painting
					   // TODO: 여기에 메시지 처리기 코드를 추가합니다.
					   // 그리기 메시지에 대해서는 CDialogEx::OnPaint()을(를) 호출하지 마십시오.
	/*
	char partialWindow[PARTIAL_WINDOW_WIDTH * PARTIAL_WINDOW_HEIGHT];
	unsigned char *p = (unsigned char *)pDlg->getPartialWindow();
	memcpy(partialWindow, p, PARTIAL_WINDOW_WIDTH * PARTIAL_WINDOW_HEIGHT);

	CString whole;
	CString part;

	for (int i = 0; i < PARTIAL_WINDOW_HEIGHT; i++)
	{
		CString value;
		CString index;
		index.Format(_T("%02d: "), i);
		part = index;
		for (int j = 0; j < PARTIAL_WINDOW_WIDTH; j++)
		{
			value.Format(_T("%02X "), *p++);
			part += value;
		}
		
		value.Format(_T("%S"), "\r\n");
		part += value;
		whole += part;
	}

	CEdit *edit = (CEdit *)GetDlgItem(IDC_EDIT_PIXEL);
	GetDlgItem(IDC_EDIT_PIXEL)->SetWindowTextW(whole);
	edit->ShowScrollBar(SB_VERT);
	//edit->LineScroll(PARTIAL_WINDOW_HEIGHT);
	*/
	unsigned char *p = (unsigned char *)pDlg->getPartialWindow();
	memcpy(m_partialWindow, p, pDlg->m_nPartialWidth * pDlg->m_nPartialHeight);

	CString whole;
	CString part;

	for (int i = 0; i < pDlg->m_nPartialHeight; i++)
	{
		CString value;
		CString index;
		index.Format(_T("%02d: "), i);
		part = index;
		for (int j = 0; j < pDlg->m_nPartialWidth; j++)
		{
			value.Format(_T("%02X "), *p++);
			part += value;
		}

		value.Format(_T("%S"), "\r\n");
		part += value;
		whole += part;
	}

	CEdit *edit = (CEdit *)GetDlgItem(IDC_EDIT_PIXEL);
	GetDlgItem(IDC_EDIT_PIXEL)->SetWindowTextW(whole);
	edit->ShowScrollBar(SB_VERT);
	
	render((unsigned char *)m_partialWindow);
}

