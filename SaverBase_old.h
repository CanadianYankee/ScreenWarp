#pragma once

#include "WindowCluster.h"
#include "DrawTimer.h"

class CSaverBase
{
public:
	CSaverBase(void);
	virtual ~CSaverBase(void);

	BOOL Initialize(const CWindowCluster::SAVER_WINDOW &myWindow);
	void Tick();
	void Pause();
	void Resume();
	void ResumeResized(int cx, int cy);
	void CleanUp();

protected:
	// Override these functions to implement your saver - return FALSE to error out and shut down
	virtual BOOL InitSaverData() = 0;
	virtual BOOL IterateSaver(SAVERFLOAT dt, SAVERFLOAT T) = 0;
	virtual BOOL PauseSaver() = 0;
	virtual BOOL ResumeSaver() = 0;
	virtual BOOL OnResize() = 0;
	virtual void CleanUpSaver() = 0;

	HWND m_hMyWindow;
	int m_iClientWidth;
	int m_iClientHeight;
	bool m_bFullScreen;
	SAVERFLOAT m_fAspectRatio;
	bool m_bRunning;
	CDrawTimer m_Timer;
};

