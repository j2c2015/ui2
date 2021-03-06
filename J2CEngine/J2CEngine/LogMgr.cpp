#include "stdafx.h"
#include "LogMgr.h"

TCHAR __sz_logMgr_version[_MAX_PATH];
TCHAR	g_szLogFuncName[MAX_LOG_FNAME];

short	g_nLogLevel = 0;
short	g_nLogWin = 0;
short	g_nLogKeepDay = 0;
long	g_lLogBackupSize = 0;

TCHAR	g_szLogPath[_MAX_PATH];
TCHAR	g_szLogFName[_MAX_FNAME];
TCHAR	g_szLogExt[_MAX_EXT];

BOOL RegisterWindowClass(HINSTANCE hInstance, LPCTSTR pszClassName)
{
	WNDCLASS wndcls;

	if (!(::GetClassInfo(hInstance, pszClassName, &wndcls)))
	{
		wndcls.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wndcls.lpfnWndProc = ::DefWindowProc;
		wndcls.cbClsExtra = 0;
		wndcls.cbWndExtra = 0;
		wndcls.hInstance = hInstance;
		wndcls.hIcon = NULL;
		wndcls.hCursor = LoadCursor(NULL, IDC_ARROW);
		wndcls.hbrBackground = (HBRUSH)(COLOR_WINDOW);
		wndcls.lpszMenuName = NULL;
		wndcls.lpszClassName = pszClassName;

		if (!RegisterClass(&wndcls))
		{
			return FALSE;
		}
	}

	return TRUE;
}

void UnregisterWindowClass(LPCTSTR pszClassName)
{
	UnregisterClass(pszClassName, NULL);
}

void FUNCTION_NAME_IS(TCHAR* pszFuncName)
{
	_tcsncpy(g_szLogFuncName, pszFuncName, MAX_LOG_FNAME);
}

void LOG_INIT(HANDLE hInstance)
{
	TCHAR szModulePath[_MAX_PATH];
	TCHAR szModuleFName[_MAX_FNAME];

	memset(szModulePath, 0x00, _MAX_PATH);
	memset(szModuleFName, 0x00, _MAX_FNAME);

	BOOL bModName = RUN_MODULE_NAME(hInstance, szModulePath, szModuleFName, NULL);
	if (!bModName)
		return;

	TCHAR szPathFName_INI[_MAX_PATH];
	memset(szPathFName_INI, 0x00, _MAX_PATH);

	_stprintf(szPathFName_INI, _T("%s%s_log.ini"), szModulePath, szModuleFName);
	if (_taccess(szPathFName_INI, 0) != 0)
	{
		g_nLogLevel = 0;
		return;
	}

	g_nLogLevel = GetPrivateProfileInt(SECTION_LOG, KEYWORD_LOG_LEVEL, 1, szPathFName_INI);

	g_nLogWin = GetPrivateProfileInt(SECTION_LOG, KEYWORD_LOG_WIN, 0, szPathFName_INI);
	if (g_nLogWin == 1)
		return;

	TCHAR szTmpPathFName[_MAX_PATH];
	memset(szTmpPathFName, 0x00, sizeof(_MAX_PATH));

	GetPrivateProfileString(SECTION_LOG, KEYWORD_LOG_FNAME, szModuleFName, szTmpPathFName, _MAX_PATH, szPathFName_INI);
	RELtoAB_PATH(hInstance, szTmpPathFName);
	TRIM_WS_FBWARD(szTmpPathFName);

	TCHAR szDrive[_MAX_DRIVE];
	TCHAR szDir[_MAX_DIR];
	TCHAR szFName[_MAX_FNAME];
	TCHAR szExt[_MAX_EXT];

	memset(szDrive, 0x00, sizeof(_MAX_DRIVE));
	memset(szDir, 0x00, sizeof(_MAX_DIR));
	memset(szFName, 0x00, sizeof(_MAX_FNAME));
	memset(szExt, 0x00, sizeof(_MAX_EXT));

	_tsplitpath(szTmpPathFName, szDrive, szDir, szFName, szExt);

	_stprintf(g_szLogPath, _T("%s%s"), szDrive, szDir);
	if (_tcslen(g_szLogPath) == 0)
		_tcscpy(g_szLogPath, szModulePath);

	if (_tcslen(szFName) == 0)
		_tcscpy(g_szLogFName, szModuleFName);
	else
		_tcscpy(g_szLogFName, szFName);

	if (_tcslen(szExt) == 0)
		_tcscpy(g_szLogExt, _T(".log"));
	else
		_tcscpy(g_szLogExt, szExt);

	g_lLogBackupSize = GetPrivateProfileInt(SECTION_LOG, KEYWORD_LOG_BACKUP_SIZE, (2 * 1024 * 1024), szPathFName_INI);

	g_nLogKeepDay = GetPrivateProfileInt(SECTION_LOG, KEYWORD_LOG_KEEP_DAY, -1, szPathFName_INI);
	if (g_nLogKeepDay > 0)
		CheckDeleteLog();
}

void LOG_PUTS(short nLogLevel, TCHAR* pszLogMsg)
{
	TCHAR szLogPathFname_Ext[_MAX_PATH];
	TCHAR szLogString[1024 * 3 * 2 + 1];
	//TCHAR szCRLF[3] = { 0x0d, 0x0a, 0x00 };
	TCHAR szCRLF[2] = { 0x0a, 0x00 };
	_SYSTEMTIME	dt;

	memset(szLogPathFname_Ext, 0x00, _MAX_PATH);
	memset(szLogString, 0x00, sizeof(szLogString));

	if (g_nLogLevel == 0 || g_nLogLevel < nLogLevel)
		return;

	GetLocalTime(&dt);
	_stprintf(szLogString, _T("%04d/%02d/%02d,%02d:%02d:%02d.%03hu, ")
		_T("%d, ")
		_T("%-33s, %s"),
		dt.wYear, dt.wMonth, dt.wDay, dt.wHour, dt.wMinute, dt.wSecond, dt.wMilliseconds,
		nLogLevel,
		g_szLogFuncName, pszLogMsg);

	if (g_nLogWin == 1)
	{
		HWND			hTrace;
		COPYDATASTRUCT	cds;

		hTrace = FindWindow(TRACEWND_CLASSNAME, NULL);
		if (hTrace)
		{
			cds.lpData = szLogString;
			cds.cbData = _tcslen(szLogString);
			SendMessage(hTrace, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
		}
	}
	else
	{
		if (g_nLogKeepDay < 0)
			_stprintf(szLogPathFname_Ext, _T("%s%s%s"), g_szLogPath, g_szLogFName, g_szLogExt);
		else
			_stprintf(szLogPathFname_Ext, _T("%s%s%04d%02d%02d%s"),
				g_szLogPath, g_szLogFName, dt.wYear, dt.wMonth, dt.wDay, g_szLogExt);

		_tcscat(szLogString, szCRLF);
		long lLogLen = _tcslen(szLogString);

		FILE*	fp = NULL;
		long	lFLen = 0;

		if ((fp = _tfopen(szLogPathFname_Ext, _T("at"))) != NULL)
		{
			//fwrite(szLogString, lLogLen, sizeof(TCHAR), fp);
			_ftprintf(fp, _T("%s"), szLogString);

			lFLen = ftell(fp);
			fclose(fp);
		}

		if (lFLen > g_lLogBackupSize)
		{
			TCHAR szTmpFName_EXT[_MAX_PATH];
			memset(szTmpFName_EXT, 0x00, _MAX_PATH);

			_stprintf(szTmpFName_EXT, _T("%s%s%04d%02d%02d_%02d%02d%02d%s"),
				g_szLogPath, g_szLogFName,
				dt.wYear, dt.wMonth, dt.wDay, dt.wHour, dt.wMinute, dt.wSecond, g_szLogExt);

			_trename(szLogPathFname_Ext, szTmpFName_EXT);
		}
	}
}

void LOG_PRINTF(TCHAR nLogLevel, TCHAR* pszFormat, ...)
{
	TCHAR szLogString[8196 + 1];
	memset(szLogString, 0x00, sizeof(szLogString));

	if (g_nLogLevel == 0 || g_nLogLevel < nLogLevel)
		return;

	va_list	vaArgs;
	va_start(vaArgs, pszFormat);

	_vstprintf(szLogString, 8196, pszFormat, vaArgs);

	LOG_PUTS(nLogLevel, szLogString);

	va_end(vaArgs);
}

FILE* LOG_PUTS_FP(short nLogLevel, FILE* fpParam, TCHAR* pszLogMsg)
{
	TCHAR szLogPathFname_Ext[_MAX_PATH];
	TCHAR szLogString[1024 * 3 * 2 + 1];
	//TCHAR szCRLF[3] = { 0x0d, 0x0a, 0x00 };
	TCHAR szCRLF[2] = { 0x0a, 0x00 };
	_SYSTEMTIME	dt;

	memset(szLogPathFname_Ext, 0x00, _MAX_PATH);
	memset(szLogString, 0x00, sizeof(szLogString));

	if (g_nLogLevel == 0 || g_nLogLevel < nLogLevel)
		return NULL;

	GetLocalTime(&dt);
	_stprintf(szLogString, _T("%04d/%02d/%02d,%02d:%02d:%02d.%03hu, ")
		_T("%d, ")
		_T("%-33s, %s"),
		dt.wYear, dt.wMonth, dt.wDay, dt.wHour, dt.wMinute, dt.wSecond, dt.wMilliseconds,
		nLogLevel,
		g_szLogFuncName, pszLogMsg);

	FILE* fp = NULL;

	if (g_nLogWin == 1)
	{
		HWND			hTrace;
		COPYDATASTRUCT	cds;

		hTrace = FindWindow(TRACEWND_CLASSNAME, NULL);
		if (hTrace)
		{
			cds.lpData = szLogString;
			cds.cbData = _tcslen(szLogString);
			SendMessage(hTrace, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
		}
	}
	else
	{
		if (g_nLogKeepDay < 0)
			_stprintf(szLogPathFname_Ext, _T("%s%s%s"), g_szLogPath, g_szLogFName, g_szLogExt);
		else
			_stprintf(szLogPathFname_Ext, _T("%s%s%04d%02d%02d%s"),
				g_szLogPath, g_szLogFName, dt.wYear, dt.wMonth, dt.wDay, g_szLogExt);

		_tcscat(szLogString, szCRLF);
		long lLogLen = _tcslen(szLogString);

		fp = fpParam;
		long	lFLen = 0;
		if (fp == NULL)
		{
			fp = _tfopen(szLogPathFname_Ext, _T("at"));
		}
		if (fp)
		{
			//fwrite(szLogString, lLogLen, sizeof(TCHAR), fp);
			_ftprintf(fp, _T("%s"), szLogString);
			lFLen = ftell(fp);
		}

		if (lFLen > g_lLogBackupSize)
		{
			TCHAR szTmpFName_EXT[_MAX_PATH];
			memset(szTmpFName_EXT, 0x00, _MAX_PATH);

			_stprintf(szTmpFName_EXT, _T("%s%s%04d%02d%02d_%02d%02d%02d%s"),
				g_szLogPath, g_szLogFName,
				dt.wYear, dt.wMonth, dt.wDay, dt.wHour, dt.wMinute, dt.wSecond, g_szLogExt);

			_trename(szLogPathFname_Ext, szTmpFName_EXT);
		}
	}
	return fp;
}

FILE* LOG_PRINTF_FP(TCHAR nLogLevel, FILE* fpParam, TCHAR* pszFormat, ...)
{
	TCHAR szLogString[8196 + 1];
	memset(szLogString, 0x00, sizeof(szLogString));

	if (g_nLogLevel == 0 || g_nLogLevel < nLogLevel)
		return NULL;

	va_list	vaArgs;
	va_start(vaArgs, pszFormat);

	_vstprintf(szLogString, 8196, pszFormat, vaArgs);

	FILE* fp = LOG_PUTS_FP(nLogLevel, fpParam, szLogString);

	va_end(vaArgs);

	return fp;
}

void CheckDeleteLog()
{
	TCHAR szLogPathFname_Ext[_MAX_PATH];

	memset(szLogPathFname_Ext, 0x00, sizeof(_MAX_PATH));

	if (g_nLogKeepDay <= 0)
		return;

	LONG lCurDate = 0;
	_SYSTEMTIME	dt;

	GetLocalTime(&dt);
	lCurDate = DATEtoLONG(dt.wYear, dt.wMonth, dt.wDay);

	_tfinddata_t c_file;
	//LONG hFile = 0;
	intptr_t hFile = 0;

	_stprintf(szLogPathFname_Ext, _T("%s%s*%s"), g_szLogPath, g_szLogFName, g_szLogExt);

	hFile = _tfindfirst(szLogPathFname_Ext, &c_file);
	if (hFile == -1L)
		return;

	while (TRUE)
	{
		DeleteOldLog(lCurDate, c_file.name, _tcslen(g_szLogFName));

		if (_tfindnext(hFile, &c_file) == -1)
		{
			_findclose(hFile);
			break;
		}
	}
}

void DeleteOldLog(long lCurDate, TCHAR* pszFName, int nPos)
{
	long lYear, lMonth, lDay;
	TCHAR szTemp[_MAX_FNAME];
	int	nLen = _tcslen(pszFName);

	memset(szTemp, 0x00, sizeof(_MAX_FNAME));

	lYear = AtoL(pszFName + nPos + 0, 4);
	lMonth = AtoL(pszFName + nPos + 4, 2);
	lDay = AtoL(pszFName + nPos + 6, 2);

	if (DATEtoLONG(lYear, lMonth, lDay) + g_nLogKeepDay < lCurDate)
	{
		_stprintf(szTemp, _T("%s%s"), g_szLogPath, pszFName);
		_tunlink(szTemp);
	}
}

void RELtoAB_PATH(HANDLE hInstance, TCHAR* pszRelPath)
{
	TCHAR szCalibratedPath[MAX_PATH] = _T("");
	if (_tcschr(pszRelPath, _T('\\')) == NULL)
		_tcscpy(szCalibratedPath, _T(".\\"));
	_tcscat(szCalibratedPath, pszRelPath);

	TCHAR szModulePath[MAX_PATH];
	TCHAR szModuleFName[_MAX_FNAME];

	memset(szModulePath, 0x00, MAX_PATH);
	memset(szModuleFName, 0x00, _MAX_FNAME);

	BOOL bModName = RUN_MODULE_NAME(hInstance, szModulePath, szModuleFName, NULL);
	if (!bModName)
		return;

	int nLen = _tcslen(szModulePath);
	if (szModulePath[nLen - 1] == _T('\\'))
		szModulePath[nLen - 1] = 0x00;

	TCHAR szABPath[MAX_PATH];
	memset(szABPath, 0x00, MAX_PATH);

	TCHAR szDrive[_MAX_DRIVE];
	TCHAR szDir[_MAX_DIR];
	TCHAR szFName[_MAX_FNAME];
	TCHAR szExt[_MAX_EXT];

	memset(szDrive, 0x00, _MAX_DRIVE);
	memset(szDir, 0x00, _MAX_DIR);

	_tsplitpath(szCalibratedPath, szDrive, szDir, szFName, szExt);

	TCHAR szSrcDirPath[MAX_PATH] = _T("");
	_stprintf(szSrcDirPath, _T("%s%s"), szDrive, szDir);
	if (_tcslen(szSrcDirPath) <= 0)
		_tcscpy(szSrcDirPath, _T(".\\"));

	// (ex) ../Temp/xxx.ext
	TCHAR* pszPath = _tcsstr(szSrcDirPath, _T(".."));
	if (pszPath)
	{
		TCHAR* pch = _tcsrchr(szModulePath, _T('\\'));
		if (pch)
		{
			_tcsncpy(szABPath, szModulePath, pch - szModulePath);

			TCHAR szRelPath_Back[MAX_PATH];
			memset(szRelPath_Back, 0x00, MAX_PATH);

			TCHAR* pchBack = _tcschr(szCalibratedPath, _T('\\'));
			if (pchBack)
			{
				_tcscpy(szRelPath_Back, pchBack);
				_tcscat(szABPath, szRelPath_Back);

				_tcscpy(pszRelPath, szABPath);
				return;
			}
		}
	}

	// (ex) ./Temp/xxx.ext
	pszPath = _tcsstr(szSrcDirPath, _T("."));
	if (pszPath)
	{
		_tcscpy(szABPath, szModulePath);

		TCHAR szRelPath_Back[MAX_PATH];
		memset(szRelPath_Back, 0x00, MAX_PATH);

		TCHAR* pchBack = _tcschr(szCalibratedPath, _T('\\'));
		if (pchBack)
		{
			_tcscpy(szRelPath_Back, pchBack);
			_tcscat(szABPath, szRelPath_Back);

			_tcscpy(pszRelPath, szABPath);
			return;
		}
	}
}


BOOL RUN_MODULE_NAME(HANDLE hInstance, TCHAR* pszPath, TCHAR* pszFName, TCHAR* pszExt)
{
	LONG lRet;
	TCHAR szPathFName[_MAX_FNAME];

	memset(szPathFName, 0x00, _MAX_FNAME);

	lRet = GetModuleFileName((HINSTANCE)hInstance, szPathFName, _MAX_FNAME);

	if (lRet == 0)
		return FALSE;

	TCHAR szDrive[_MAX_DRIVE];
	TCHAR szDir[_MAX_DIR];

	memset(szDrive, 0x00, _MAX_DRIVE);
	memset(szDir, 0x00, _MAX_DIR);

	_tsplitpath(szPathFName, szDrive, szDir, pszFName, pszExt);
	_stprintf(pszPath, _T("%s%s"), szDrive, szDir);

	return TRUE;
}

void TRIM_WS_FBWARD(TCHAR* pszString)
{
	int nLen = _tcslen(pszString);
	if (nLen <= 0)
		return;

	TCHAR* pszTrimString = new TCHAR[nLen + 1];
	memset(pszTrimString, 0x00, sizeof(TCHAR) * (nLen + 1));

	int i = 0;
	int nLastChar = nLen - 1;
	for (i = nLen - 1; i >= 0; i--)
	{
		if (pszString[i] != 0x20 &&
			pszString[i] != 0x09 && pszString[i] != 0x0B &&
			pszString[i] != 0x0A && pszString[i] != 0x0D)
		{
			nLastChar = i;
			break;
		}
	}

	BOOL bFindChar = FALSE;
	int j = 0;
	for (i = 0; i <= nLastChar; i++)
	{
		if (!bFindChar &&
			pszString[i] != 0x20 &&
			pszString[i] != 0x09 && pszString[i] != 0x0B &&
			pszString[i] != 0x0A && pszString[i] != 0x0D)
		{
			bFindChar = TRUE;
		}

		if (bFindChar)
			pszTrimString[j++] = pszString[i];
	}

	if (j > 0)
	{
		_tcscpy(pszString, pszTrimString);
		pszString[j] = NULL;
	}

	delete[] pszTrimString;
}

BOOL IsEunYearL(long lYear)
{
	return (lYear % 4 == 0) && ((lYear % 100 != 0) || (lYear % 400 == 0));
}

long DATEtoLONG(long lYear, long lMonth, long lDay)
{
	long lMonthTotDays[12] = { 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
	long lTotalDays = 0L;

	--lYear;
	--lMonth;

	lTotalDays = (lYear * 365) + (lYear / 4) - (lYear / 100) + (lYear / 400);

	if (lMonth > 0)
	{
		lTotalDays += lMonthTotDays[lMonth - 1];
		if (IsEunYearL(lYear + 1) && (lMonth > 1))
			lTotalDays++;
	}

	return (lTotalDays + lDay);
}

long AtoL(TCHAR* pszData, short nLen)
{
	TCHAR szTemp[20];
	memset(szTemp, 0x00, sizeof(szTemp));

	memcpy(szTemp, pszData, sizeof(TCHAR) * nLen);
	szTemp[nLen] = 0x00;

	return _ttoi(szTemp);
}

CEllapsedTime::CEllapsedTime()
{
	m_fEllapsedTime = 0.0;
}

CEllapsedTime::~CEllapsedTime()
{
	//
}

void CEllapsedTime::StartEllapsedTime()
{
	QueryPerformanceFrequency(&m_liFrequency);
	QueryPerformanceCounter(&m_liCounter1);
}

double CEllapsedTime::EndEllapsedTime()
{
	QueryPerformanceCounter(&m_liCounter2);

	if (m_liFrequency.QuadPart > 0.0)
		m_fEllapsedTime = (double)(m_liCounter2.QuadPart - m_liCounter1.QuadPart) / (double)m_liFrequency.QuadPart;

	return m_fEllapsedTime;
}
