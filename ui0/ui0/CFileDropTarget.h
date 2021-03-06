
#pragma once
#include "afxole.h"

// CFileDropTarget command target

class CFileDropTarget : public COleDropTarget
{
public:
	CFileDropTarget();
	virtual ~CFileDropTarget();

	virtual DROPEFFECT OnDragEnter(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	virtual DROPEFFECT OnDragOver(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	virtual BOOL OnDrop(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);

private:
	DROPEFFECT GetDropEffect(COleDataObject* pDataObject);

};


