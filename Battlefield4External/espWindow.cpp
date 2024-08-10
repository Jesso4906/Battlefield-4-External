#include "espWindow.h"

RECT rectangles[64];
void ClearRects() { memset(rectangles, 0, sizeof(RECT) * 64); }
void SetRect(int index, RECT rect) { rectangles[index] = rect; }

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT paintStruct;
	HDC hDC;
	HBRUSH hBrush;
	switch (message)
	{
	case WM_CREATE:
		return 0;
		break;
	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;
		break;
	case WM_PAINT:
		hDC = BeginPaint(hwnd, &paintStruct);
		hBrush = CreateSolidBrush(RGB(0, 255, 0));

		for (int i = 0; i < 64; i++)
		{
			if (rectangles[i].left == 0) { continue; }
			FrameRect(hDC, &rectangles[i], hBrush);
		}

		EndPaint(hwnd, &paintStruct);
		return 0;
		break;
	default:
		break;
	}
	return (DefWindowProc(hwnd, message, wParam, lParam));
}

DWORD WINAPI ThreadProc(LPVOID lpParameter) 
{
	WNDCLASSEX windowClass;

	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WndProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.hbrBackground = CreateSolidBrush(0x000000FF); // red
	windowClass.lpszMenuName = NULL;
	windowClass.lpszClassName = L"EspClass";
	windowClass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);

	if (!RegisterClassEx(&windowClass)) { return 0; }

	HWND hwnd = CreateWindowEx(WS_EX_TOPMOST,//extended style
		L"EspClass",//class name
		L"Esp",//app name
		WS_VISIBLE | WS_POPUP,//window style
		0, 0,//x/y coords
		1, 1,//width,height
		NULL,//handle to parent
		NULL,//handle to menu
		NULL,//application instance
		NULL);//no extra parameters

	SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED | WS_EX_TRANSPARENT);
	SetLayeredWindowAttributes(hwnd, RGB(255, 0, 0), 0, LWA_COLORKEY); // red pixels will be transparent

	BlockInput(TRUE);
	
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!IsWindow(hwnd)) { break; }
		
		if (!IsDialogMessage(hwnd, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return 0;
}

HWND CreateEspWindow()
{
	CreateThread(NULL, 0, ThreadProc, NULL, 0, NULL);

	Sleep(10);

	WindowHandleData data;
	data.procId = GetCurrentProcessId();
	data.handle = 0;

	EnumWindows(EnumWindowCallback, (LPARAM)&data);

	return data.handle;
}

BOOL CALLBACK EnumWindowCallback(HWND hwnd, LPARAM lparam)
{
	WindowHandleData& data = *(WindowHandleData*)lparam;
	DWORD procId = 0;
	GetWindowThreadProcessId(hwnd, &procId);

	if (data.procId != procId || !IsWindowVisible(hwnd) || GetConsoleWindow() == hwnd) { return TRUE; }

	data.handle = hwnd;
	return FALSE;
}


HWND GetBF4Window(DWORD bf4ProcId)
{
	WindowHandleData data;
	data.procId = bf4ProcId;
	data.handle = 0;

	EnumWindows(EnumWindowCallback, (LPARAM)&data);

	return data.handle;
}