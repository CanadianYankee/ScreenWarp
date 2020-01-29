#pragma once

class CSaverEngine;
class CSaverBase;

class CWindowCluster
{
public:
	CWindowCluster(CSaverEngine *pEngine);
	virtual ~CWindowCluster(void);

	static CWindowCluster *s_pTheCluster;
	LRESULT ClusterWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	BOOL IsMultiRender() { return m_bMultiRender; }
	BOOL AddPrimary(LPRECT lpRect, BOOL bSystemPrimary);
	BOOL AddSecondary(LPRECT lpRect);
	BOOL AddNested(HWND hParent);

	BOOL CreateAll();
	void Run();
	void DestroyAll();


	typedef struct tagSAVER_WINDOW
	{
		RECT rcScreenRect;
		RECT rcCreateArea;
		SIZE sClientArea;
		DWORD dwCreateFlags;
		bool bSaverMode;
		bool bNested;
		bool bPrimary;
		HWND hParentWnd;
		HWND hWnd;
		CSaverBase *pSaver;
	} SAVER_WINDOW;

protected:
	typedef std::vector<SAVER_WINDOW> WindowVector;
	typedef WindowVector::iterator WindowIter;

	void RegisterWindowClass(SAVER_WINDOW windowInfo, const std::wstring &strClass);
	WindowIter LookupWindow(HWND hWnd);

	CSaverEngine *m_pParentEngine;
	WindowVector m_vecWindows;
	BOOL m_bMultiRender;

	HBRUSH m_blackBrush;
	POINT m_ptMouse;
	bool m_bResizing;
};

