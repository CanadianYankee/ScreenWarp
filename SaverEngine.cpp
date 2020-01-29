#include "stdafx.h"
#include "SaverEngine.h"
#include "WindowCluster.h"
#include "ConfigDialog.h"

BOOL CALLBACK MonitorEnumFn(HMONITOR hMonitor, HDC hDC, LPRECT pRect, LPARAM lParam)
{
	return ((CSaverEngine *)lParam)->PerMonitorInit(hMonitor);
}

CSaverEngine::CSaverEngine(HINSTANCE hAppInstance) :
	m_hAppInstance(hAppInstance),
	m_sdmMode(sdmUnknown),
	m_hParentWnd(nullptr),
	m_pWindowCluster(nullptr)
{
	srand(GetTickCount());
}

CSaverEngine::~CSaverEngine(void)
{
}

void CSaverEngine::CleanUp()
{
	if(m_pWindowCluster)
	{
		m_pWindowCluster->DestroyAll();
		delete m_pWindowCluster;
		m_pWindowCluster = nullptr;
	}
}

BOOL CSaverEngine::Initialize(LPTSTR lpCmdLine)
{
	// Figure out what our display/windowing mode should be
	ParseForOptions(lpCmdLine);
	if(m_sdmMode == sdmUnknown)
		return false;

	// Set up the window(s), unless we're doing config only
	if(m_sdmMode == sdmConfigOnly)
	{
		DoConfig();

		return FALSE;	// Everything is done, no need to run the saver.
	}

	BOOL bSuccess = SetupWindows();

	return bSuccess;
}

#define MAXFNAME 1024
void CSaverEngine::ParseForOptions(LPCTSTR lpCmdLine)
{
	// Figure out the file extension ("scr" is a screen saver)
	bool bSCR = false;
	bool bEXE = false;
	TCHAR fname[MAXFNAME];
	::GetModuleFileName(NULL, fname, MAXFNAME);
	size_t fnlen = _tcslen(fname);
	if(fnlen > 4)
	{
		TCHAR *fext = fname + (fnlen-4);
		bSCR = _tcsicmp(fext, L".scr") == 0;
		bEXE = _tcsicmp(fext, L".exe") == 0;
	}
	assert(bSCR || bEXE);
	
	// Set for either "saver" or "windowed" mode; may change after parsing options
	m_sdmMode = bSCR ? sdmSaverWindow : sdmNormalWindow;
	
	TCHAR tcOption;
	HANDLE hParam;
	bool bParsed = ParseOption(lpCmdLine, tcOption, hParam);
	if(bSCR && !bParsed)
	{
		tcOption = L'c';
		bParsed = true;
	}
	if(bParsed)
	{
		switch(tcOption)
		{
		case _T('c'):
			m_sdmMode = sdmConfigOnly;
			if(hParam)
			{
				m_hParentWnd = (HWND)hParam;
			}
			break;

		case _T('s'):
			m_sdmMode = sdmSaverWindow;
			break;

		case _T('p'):
			m_sdmMode = sdmPreviewWindow;
			if(hParam)
			{
				m_hParentWnd = (HWND)hParam;
			}
			break;
		}
	}
}

bool CSaverEngine::ParseOption(LPCTSTR lpCmdLine, TCHAR &tcOpt, HANDLE &hParam) const
{
	tcOpt = _T(' ');
	hParam = nullptr;

	size_t cmdLength = _tcslen(lpCmdLine);
	if(cmdLength == 0)
		return false;

	LPCTSTR lpsz = lpCmdLine;
	if(lpsz[0] == _T('-') || lpsz[0] == _T('/'))
	{
		lpsz++;
		if(--cmdLength < 1) 
			return false;
	}

	TCHAR szOpt[2];
	_tcsncpy_s(szOpt, lpsz, 1);
	CharLower(szOpt);
	if(IsCharAlpha(szOpt[0]))
	{
		tcOpt = szOpt[0];
		lpsz++;
		if(--cmdLength < 1)
			return true;
	}
	else
	{
		return false;
	}

	if(--cmdLength < 1) 
		return true;
	lpsz++;

	_stscanf_s(lpsz, L"%ld", &hParam);
	
	return true;
}

BOOL CSaverEngine::SetupWindows()
{
	assert(!m_pWindowCluster);
	m_pWindowCluster = new CWindowCluster(this);

	BOOL bSuccess = FALSE;

	if(m_sdmMode == sdmPreviewWindow && m_hParentWnd)
	{
		// Create a nested, frameless child window inside the provided parent
		bSuccess = m_pWindowCluster->AddNested(m_hParentWnd);
	}
	else
	{
		// Create the main window on the main monitor, plus dummy ones to cover up the 
		// secondary displays in saver mode
		bSuccess = EnumDisplayMonitors(NULL, NULL, MonitorEnumFn, (LPARAM)this);
	}

	if(bSuccess)
	{
		bSuccess = m_pWindowCluster->CreateAll();
	}

	return bSuccess;
}

BOOL CSaverEngine::PerMonitorInit(HMONITOR hMonitor)
{
	MONITORINFO monitorInfo;
	ZeroMemory(&monitorInfo, sizeof(monitorInfo));
	monitorInfo.cbSize = sizeof(monitorInfo);
	if(!GetMonitorInfo(hMonitor, &monitorInfo))
		return FALSE;

	if(IsRectEmpty(&monitorInfo.rcMonitor))
		return TRUE;

	BOOL bSuccess = TRUE;
	if((monitorInfo.dwFlags & MONITORINFOF_PRIMARY) || m_pWindowCluster->IsMultiRender())
	{
		bSuccess = m_pWindowCluster->AddPrimary(&monitorInfo.rcMonitor, monitorInfo.dwFlags & MONITORINFOF_PRIMARY);
	}
	else if(m_sdmMode == sdmSaverWindow)
	{
		bSuccess = m_pWindowCluster->AddSecondary(&monitorInfo.rcMonitor);
	}

	return bSuccess;
}

void CSaverEngine::Run()
{
	m_pWindowCluster->Run();
}

void CSaverEngine::DoConfig()
{
	CConfigDialog dlg;

	dlg.ShowModal(m_hAppInstance, m_hParentWnd);
}
