#pragma once

class CConfigDialog
{
public:
	CConfigDialog(void);
	~CConfigDialog(void);

	INT_PTR ShowModal(HINSTANCE hAppInstance, HWND hWndParent);
	INT_PTR ConfigDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static CConfigDialog *s_pActiveConfig;
};

