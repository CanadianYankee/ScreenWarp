#pragma once

class CWindowCluster;
class CSecondarySaverWindows;

#define WM_STOPSAVER (WM_USER + 0x87a)

class CSaverEngine 
{
public:
	enum SaverDisplayMode
	{
		sdmUnknown = 0,
		sdmNormalWindow,
		sdmSaverWindow,
		sdmPreviewWindow,
		sdmConfigOnly
	};

	CSaverEngine(HINSTANCE hAppInstance);
	~CSaverEngine(void);

	SaverDisplayMode GetMode() const { return m_sdmMode; }
	HINSTANCE GetAppInstance() const { return m_hAppInstance; } 

	BOOL Initialize(LPTSTR lpCmdLine);
	BOOL PerMonitorInit(HMONITOR hMonitor);

	void Run();
	void DoConfig();

	void CleanUp();

private:
	void ParseForOptions(LPCTSTR lpCmdLine);
	bool ParseOption(LPCTSTR lpCmdLine, TCHAR &tcOpt, HANDLE &hParam) const;
	BOOL SetupWindows();

	HINSTANCE m_hAppInstance;
	HWND m_hParentWnd;
	CWindowCluster *m_pWindowCluster;
	SaverDisplayMode m_sdmMode;
};

