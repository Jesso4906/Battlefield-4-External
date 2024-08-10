#pragma once
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#include <vector>

struct WindowHandleData
{
	DWORD procId;
	HWND handle;
};

void ClearRects();

void SetRect(int index, RECT rect);

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

DWORD WINAPI ThreadProc(LPVOID lpParameter);

HWND CreateEspWindow();

BOOL CALLBACK EnumWindowCallback(HWND hwnd, LPARAM lparam);

HWND GetBF4Window(DWORD bf4ProcId);