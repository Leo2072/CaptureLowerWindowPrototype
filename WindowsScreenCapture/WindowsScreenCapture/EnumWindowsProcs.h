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

struct GetWndRgnFillData
{
	HRGN AvailableRegion;
	std::vector<HRGN>* TransferRegions;
	std::vector<HWND>* SourceHWNDs;
	std::vector<POINT>* Offsets;
};


BOOL CALLBACK FilterEnumWindows(HWND CurrentWindow, LPARAM lParam);


BOOL CALLBACK GetLowerWndRgn(HWND CurrentWindow, LPARAM lParam);