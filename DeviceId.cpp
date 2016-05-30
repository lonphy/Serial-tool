#include "resource.h"
#include "Worker.h"
#include <Dbt.h>

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' \
version='6.0.0.0' \
processorArchitecture='*' \
publicKeyToken='6595b64144ccf1df' \
language='*'\"")

#define ID_MENU_LOG_CLEAR          100001 
#define MAX_LOG_INFO_LEN           1024*1024

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);



HINSTANCE hgAppInst;
HWND gHdlg;
DWORD dwWorkerID;
WNDPROC defLogProc;

char logBuf[MAX_LOG_INFO_LEN];
char numberBuf[5];

int WINAPI WinMain(HINSTANCE hThisApp, HINSTANCE hPrevApp, LPSTR lpCmd, int nShow)
{
#if 1
	AllocConsole(); // 创建一个控制台窗口
	freopen("CONOUT$", "w", stdout); // 并打开
#endif

	InitHttp();


	// 设计窗口类
	WNDCLASS wc = { };
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpfnWndProc = (WNDPROC)WindowProc;
	wc.lpszClassName = "DeviceId";
	wc.hInstance = hThisApp;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.hIcon = LoadIcon(hThisApp, MAKEINTRESOURCE(IDI_ICON1));
	RegisterClass(&wc);
	hgAppInst = hThisApp;
	
	// 创建窗口
	HWND hwnd = CreateWindow(
		"DeviceId",
		"设备号烧写工具-v1.0.2", 
		(WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU),
		CW_USEDEFAULT, CW_USEDEFAULT,
		520, 480,
		NULL, NULL, hThisApp, NULL);
	if(!hwnd)
		return 0;
	ShowWindow(hwnd, nShow);
	UpdateWindow(hwnd);

	// 消息循环
	MSG msg;
	while(GetMessage(&msg,NULL,0,0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

#if 1
	system("pause");
	FreeConsole();   // 释放控制台窗口
#endif
	DestoryWorker();
	DestoryHttp();
	return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CREATE:
			gHdlg = CreateDialog(hgAppInst, MAKEINTRESOURCE(IDD_MYFORM), hwnd, (DLGPROC)DlgProc);
			dwWorkerID = InitWorker(gHdlg);
			ShowWindow(gHdlg, SW_SHOWNA);
		return 0;
	case WM_DEVICECHANGE:
		switch(wParam)
		{
		case DBT_DEVICEARRIVAL:
			if( ((PDEV_BROADCAST_HDR)lParam)->dbch_devicetype == DBT_DEVTYP_PORT )
			{
				printf("有串口设备接入%s\r\n", ((PDEV_BROADCAST_PORT)lParam)->dbcp_name);
			}
			break;
		case DBT_DEVICEREMOVECOMPLETE:
			if( ((PDEV_BROADCAST_HDR)lParam)->dbch_devicetype == DBT_DEVTYP_PORT )
			{
				printf("%s连接设备被拔出了\r\n", ((PDEV_BROADCAST_PORT)lParam)->dbcp_name);
			}
			break;
		}
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd,msg,wParam,lParam);
}

void LOG(const char * msg)
{
	strcat(logBuf, msg);
	SetWindowText( GetDlgItem(gHdlg, IDC_LOG), logBuf);
	SendMessage(GetDlgItem(gHdlg, IDC_LOG), WM_VSCROLL, SB_BOTTOM, 0);
}

LRESULT CALLBACK logProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	POINT p;
	static HMENU menu = 0;
	switch(message)
	{
	case WM_COMMAND:
		if (wParam == ID_MENU_LOG_CLEAR)
		{
			SetWindowText(hwnd, 0);
			memset(logBuf, 0, MAX_LOG_INFO_LEN);
		}
		break;
	case WM_RBUTTONDOWN:
		if (menu == 0)
		{
			menu = CreatePopupMenu();
			AppendMenu(menu, MF_STRING, ID_MENU_LOG_CLEAR, "清除");
		}
		GetCursorPos(&p);
		// ScreenToClient(hwnd, &p);
		TrackPopupMenu(menu, TPM_LEFTALIGN, p.x, p.y, 0, hwnd, 0);
		return 0;
	case WM_DESTROY:
		DestroyMenu(menu);
	}
	return CallWindowProc(defLogProc, hwnd, message, wParam, lParam);
}

// 处理对话框消息
INT_PTR CALLBACK DlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HDC hdcStatic;
	static HBRUSH hBrush;
	char* deviceId = 0;
	static char comx[5];
	static int comxSel = 0;

	switch(msg)
	{
	case WM_INITDIALOG:
		hBrush = CreateSolidBrush( RGB(255,255,255) );
		// 初始化下拉列表框
		SendDlgItemMessage(hdlg, IDC_COMBO_COMS, CB_ADDSTRING, 0, (LPARAM)"选择COM");
		SendDlgItemMessage(hdlg, IDC_COMBO_COMS, CB_ADDSTRING, 0, (LPARAM)"COM1");
        SendDlgItemMessage(hdlg, IDC_COMBO_COMS, CB_ADDSTRING, 0, (LPARAM)"COM2");
        SendDlgItemMessage(hdlg, IDC_COMBO_COMS, CB_ADDSTRING, 0, (LPARAM)"COM3");
        SendDlgItemMessage(hdlg, IDC_COMBO_COMS, CB_ADDSTRING, 0, (LPARAM)"COM4");
		SendDlgItemMessage(hdlg, IDC_COMBO_COMS, CB_ADDSTRING, 0, (LPARAM)"COM5");
        SendDlgItemMessage(hdlg, IDC_COMBO_COMS, CB_SETCURSEL, 0, 0);
		memset(comx, 0, 5);

		defLogProc = (WNDPROC)SetWindowLong( GetDlgItem(hdlg, IDC_LOG), GWL_WNDPROC, (LONG)logProc );
		break;
	case WM_DESTROY:
		DeleteObject(hBrush);
		break;
	case WM_CTLCOLORSTATIC:
		if (HWND(lParam) == GetDlgItem(hdlg, IDC_STATIC_WEB) )
		{
			hdcStatic = (HDC)wParam;
			SetTextColor( hdcStatic, RGB(0x33, 0x66, 0x99) );
			SetBkMode( hdcStatic, TRANSPARENT );
			return (INT_PTR) hBrush;
		}
		break;
	case WM_LOG_MSG:
		switch(wParam)
		{
		case 2: // 成功
		case 1: // 失败
			EnableWindow(GetDlgItem(hdlg, IDC_SAVE), TRUE);
		}
		LOG((const char *)lParam);
		return (INT_PTR)1;
	case WM_COUNT_MSG: // 更新计数
		memset(numberBuf, 0, 5);
		sprintf(numberBuf, "%d", wParam);
		SetWindowText( GetDlgItem(hdlg, IDC_STATIC_TOTAL), numberBuf);
		memset(numberBuf, 0, 5);
		sprintf(numberBuf, "%d", lParam);
		SetWindowText( GetDlgItem(hdlg, IDC_STATIC_SUCCESS), numberBuf);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_COMBO_COMS:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				if (0 < (comxSel = SendDlgItemMessage(gHdlg, IDC_COMBO_COMS, CB_GETCURSEL, 0, 0)))
				{
					EnableWindow(GetDlgItem(hdlg, IDC_COMBO_COMS), FALSE);
					sprintf(comx, "COM%d", comxSel);
					PostThreadMessage(dwWorkerID, WM_CHANGE_COMPORT, (WPARAM)comx, 0);
				}
			}
			break;
		case IDC_SAVE:
			if (comxSel > 0) {
				EnableWindow(GetDlgItem(hdlg, IDC_SAVE), FALSE);
				deviceId = new char[17];
				GetWindowText( GetDlgItem(hdlg, IDC_DEVICEID), deviceId, 17);
				PostThreadMessage(dwWorkerID, WM_SERIAL_WRITE, (WPARAM)deviceId, 0);
			}
			else
			{
				MessageBox(hdlg, "请先选择COM口！！！", "警告", MB_OK);
			}
			break;
		case IDC_STATIC_WEB:
			ShellExecute(0, "open", "http://reporturl.com/", 0, 0, SW_SHOW);
			break;
		default:
			return DefWindowProc(hdlg, msg, wParam, lParam);
		}
		break;
	}
	return (INT_PTR)FALSE;
}
