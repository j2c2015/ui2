// CIdentifyDialog.cpp: 구현 파일
//

#include "stdafx.h"
#include "ui0.h"
#include "CIdentifyDialog.h"
#include "afxdialogex.h"
#include "ui0Dlg.h"

CIdentifyDialog *pIdentifyDlg=0;
// CIdentifyDialog 대화 상자

IMPLEMENT_DYNAMIC(CIdentifyDialog, CDialogEx)

CIdentifyDialog::CIdentifyDialog(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_IDENTIFY, pParent)
{

}

CIdentifyDialog::~CIdentifyDialog()
{
}

void CIdentifyDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CIdentifyDialog, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_IDENTIFY, &CIdentifyDialog::OnBnClickedButtonIdentify)
	ON_WM_PAINT()
END_MESSAGE_MAP()


// CIdentifyDialog 메시지 처리기
void __identifyCb(char *id, bool success)
{
	if (success == true)
	{
		CString str;
		CString name;
		name.Format(_T("%S"), id);
		pIdentifyDlg->setIdentifyText(name);
		str.Format(_T("Found. %S"), id);
		AfxMessageBox(str, MB_OK);
	}
	else
	{
		AfxMessageBox(_T("Not found"), MB_OK);
	}
}

void CIdentifyDialog::OnBnClickedButtonIdentify()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	identifyRequest(::__identifyCb);
}

void CIdentifyDialog::render(unsigned char *pBuffer)
{
	HRESULT lRet;

	if (pDlg->m_pDirect3DSurfaceRenderForIdentify == NULL)
		return;

	D3DLOCKED_RECT d3d_rect;
	lRet = pDlg->m_pDirect3DSurfaceRenderForIdentify->LockRect(&d3d_rect, NULL, D3DLOCK_DONOTWAIT);
	if (FAILED(lRet))
		return;

	byte *pSrc = pBuffer;
	byte * pDest = (BYTE *)d3d_rect.pBits;
	int stride = d3d_rect.Pitch;
	unsigned long nIndex = 0;

	memset(pDest, 0x80, (stride)*HEIGHT_IDENTIFY_EYE_VIEW*BPP / 8);
	for (nIndex = 0; nIndex < HEIGHT_IDENTIFY_EYE_VIEW; nIndex++) {
		memcpy(pDest + nIndex * stride, pSrc + nIndex * WIDTH_IDENTIFY_EYE_VIEW, WIDTH_IDENTIFY_EYE_VIEW);
	}

	lRet = pDlg->m_pDirect3DSurfaceRenderForIdentify->UnlockRect();
	if (FAILED(lRet))
		return;

	if (pDlg->m_pDirect3DDeviceForIdentify == NULL)
		return;

	pDlg->m_pDirect3DDeviceForIdentify->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	pDlg->m_pDirect3DDeviceForIdentify->BeginScene();
	IDirect3DSurface9 * pBackBuffer = NULL;

	pDlg->m_pDirect3DDeviceForIdentify->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
	pDlg->m_pDirect3DDeviceForIdentify->StretchRect(pDlg->m_pDirect3DSurfaceRenderForIdentify, NULL, pBackBuffer, &pDlg->m_IdentifyRenderViewport, D3DTEXF_LINEAR);
	pDlg->m_pDirect3DDeviceForIdentify->EndScene();
	pDlg->m_pDirect3DDeviceForIdentify->Present(NULL, NULL, NULL, NULL);
}


bool CIdentifyDialog::initView()
{
	HWND hwnd = GetDlgItem(IDC_IDENTIFY_RENDER_VIEW)->m_hWnd;
	bool	bSuccess = false;
	HRESULT hrSuccess;

	D3DPRESENT_PARAMETERS d3dpp;
	D3DFORMAT Format;
	__try
	{
		m_pDirect3D9 = Direct3DCreate9(D3D_SDK_VERSION);
		ZeroMemory(&d3dpp, sizeof(d3dpp));
		d3dpp.Windowed = TRUE;
		d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

		::GetClientRect(hwnd, &pDlg->m_IdentifyRenderViewport);
		hrSuccess = m_pDirect3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDlg->m_pDirect3DDeviceForIdentify);
		if (FAILED(hrSuccess))
		{
			__leave;
		}

		Format = (D3DFORMAT)MAKEFOURCC('Y', 'V', '1', '2');	// J2C
		hrSuccess = pDlg->m_pDirect3DDeviceForIdentify->CreateOffscreenPlainSurface(WIDTH_IDENTIFY_EYE_VIEW, HEIGHT_IDENTIFY_EYE_VIEW, Format, D3DPOOL_DEFAULT, &pDlg->m_pDirect3DSurfaceRenderForIdentify, NULL);
		if (FAILED(hrSuccess))
		{
			__leave;
		}

		bSuccess = true;
	}
	__finally
	{
		if (false == bSuccess)
		{
			pDlg->cleanup();
		}
	}

	return bSuccess;
}
// CConfigDialog 메시지 처리기
void __identifyRender(void *buffer, int width, int height, int depth)
{
	pIdentifyDlg->render((unsigned char *)buffer);
}
void CIdentifyDialog::setIdentifyText(CString &str)
{
	GetDlgItem(IDC_EDIT1)->SetWindowTextW(str);
}

BOOL CIdentifyDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  여기에 추가 초기화 작업을 추가합니다.
	pIdentifyDlg = this;
	initView();
	setIdentifyRenderer(::__identifyRender);
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}


void CIdentifyDialog::OnPaint()
{
	CPaintDC dc(this); // device context for painting
					   // TODO: 여기에 메시지 처리기 코드를 추가합니다.
					   // 그리기 메시지에 대해서는 CDialogEx::OnPaint()을(를) 호출하지 마십시오.
	CString name = _T("");
	setIdentifyText(name);
}
