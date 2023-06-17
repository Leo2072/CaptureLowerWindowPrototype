#pragma once
#include <Windows.h>
#include <stdlib.h>
#include <vector>

#define EnumWindowsProcPtr(name) BOOL(*name)(HWND, LPARAM)


struct FilterProcData
{
	HWND LowestHWND = 0;
	HWND HighestHWND = 0;
	EnumWindowsProcPtr(ProcPtr) = nullptr;
	LPARAM lParam = 0;

	bool ExcludeMax = false;
	bool ExcludeMin = false;

	bool HigherThanMax = true;
	bool LowerThanMin = false;
};


BOOL CALLBACK FilterEnumWindows(HWND CurrentWindow, LPARAM lParam);