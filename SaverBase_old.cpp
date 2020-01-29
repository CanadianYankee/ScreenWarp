#include "stdafx.h"
#include "SaverBase.h"

CSaverBase::CSaverBase(void) :
	m_hMyWindow(NULL),
	m_bRunning(false)
{
}


CSaverBase::~CSaverBase(void)
{
}

BOOL CSaverBase::Initialize(const CWindowCluster::SAVER_WINDOW &myWindow)
{
	if(!myWindow.hWnd)
	{
		assert(false);
		return FALSE;
	}

	m_hMyWindow = myWindow.hWnd;
	m_iClientWidth = myWindow.sClientArea.cx;
	m_iClientHeight = myWindow.sClientArea.cy;
	m_fAspectRatio = (SAVERFLOAT)m_iClientWidth/(SAVERFLOAT)m_iClientHeight;
	m_bFullScreen = myWindow.bSaverMode;

	m_Timer.Reset();
	
	BOOL bSuccess = InitSaverData();
	if(bSuccess)
		m_bRunning = true;

	return bSuccess;
}

void CSaverBase::Tick()
{
	if(m_bRunning)
	{
		m_Timer.Tick();

		if(!IterateSaver(m_Timer.DeltaTime(), m_Timer.TotalTime()))
		{
			PostMessage(m_hMyWindow, WM_DESTROY, 0, 0);
		}
	}
	else
	{
	}
}

void CSaverBase::Pause()
{
	if(m_bRunning)
	{
		if(!PauseSaver())
		{
			PostMessage(m_hMyWindow, WM_DESTROY, 0, 0);
		}
		m_bRunning = false;
		m_Timer.Stop();
	}
}

void CSaverBase::Resume()
{
	if(!m_bRunning)
	{
		if(!ResumeSaver())
		{
			PostMessage(m_hMyWindow, WM_DESTROY, 0, 0);
		}
		m_bRunning = true;
		m_Timer.Start();
	}
}


void CSaverBase::ResumeResized(int cx, int cy)
{
	if(m_bRunning)
		Pause();

	m_iClientWidth = cx;
	m_iClientHeight = cy;
	m_fAspectRatio = (SAVERFLOAT)m_iClientWidth/(SAVERFLOAT)m_iClientHeight;

	if(OnResize())
	{
		Resume();
	}
	else
	{
		PostMessage(m_hMyWindow, WM_DESTROY, 0, 0);
	}
}

void CSaverBase::CleanUp()
{
	if(m_bRunning)
		Pause();

	CleanUpSaver();
}


