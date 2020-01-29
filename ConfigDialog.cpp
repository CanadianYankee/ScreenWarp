#include "stdafx.h"
#include "Resource.h"
#include "ConfigDialog.h"

CConfigDialog *CConfigDialog::s_pActiveConfig = nullptr;

INT_PTR CALLBACK gConfigDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	assert(CConfigDialog::s_pActiveConfig != nullptr);
	return CConfigDialog::s_pActiveConfig->ConfigDlgProc(hwndDlg, uMsg, wParam, lParam);
}


CConfigDialog::CConfigDialog(void)
{
}


CConfigDialog::~CConfigDialog(void)
{
}

INT_PTR CConfigDialog::ShowModal(HINSTANCE hAppInstance, HWND hWndParent)
{
	assert(s_pActiveConfig == nullptr);
	s_pActiveConfig = this;
	INT_PTR iResult = DialogBox(hAppInstance, MAKEINTRESOURCE(IDD_CONFIGDLG), hWndParent, gConfigDlgProc);
	s_pActiveConfig = nullptr;

	return iResult;
}

INT_PTR CConfigDialog::ConfigDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) 
    { 
        case WM_COMMAND: 
            switch (LOWORD(wParam)) 
            { 
                case IDOK: 
                case IDCANCEL: 
                    EndDialog(hwndDlg, wParam); 
                    return TRUE; 
            } 
    } 
    return FALSE; 
}
