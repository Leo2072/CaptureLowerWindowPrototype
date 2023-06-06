#include "EnumWindowsProcs.h"


BOOL CALLBACK FilterEnumWindows(HWND CurrentWindow, LPARAM lParam)
{
	FilterProcData* FilterData = (FilterProcData*) lParam;
	if (FilterData->LowerThanMin) return FALSE;
	if (!FilterData->HigherThanMax)
	{
		if (FilterData->LowestHWND != 0 && FilterData->LowestHWND == CurrentWindow)
		{
			FilterData->LowerThanMin = true;
            if (FilterData->ExcludeMin) return FALSE;
		}
		if (!FilterData->ProcPtr(CurrentWindow, FilterData->lParam)) return FALSE;
	}
	else if (FilterData->HighestHWND == 0 || FilterData->HighestHWND == CurrentWindow)
	{
		FilterData->HigherThanMax = false;
        if (!FilterData->ExcludeMax)
        {
            if (!FilterData->ProcPtr(CurrentWindow, FilterData->lParam)) return FALSE;
        }
	}
	return TRUE;
}


BOOL CALLBACK GetLowerWndRgn(HWND CurrentWindow, LPARAM lParam)
{
	GetWndRgnFillData* FillData = (GetWndRgnFillData*) lParam;
    RECT CurrentRect;
    if (IsWindowVisible(CurrentWindow))
    {
        if (GetWindowRect(CurrentWindow, &CurrentRect) != FALSE)
        {
            HRGN CurrentRegion(CreateRectRgnIndirect(&CurrentRect));
            int GetRgnResult = CombineRgn(CurrentRegion, FillData->AvailableRegion, CurrentRegion, RGN_AND);
            if (GetRgnResult != ERROR && GetRgnResult != NULLREGION)
            {
                POINT CurrentOffset { CurrentRect.left, CurrentRect.top };
                FillData->TransferRegions->push_back(CurrentRegion);
                FillData->SourceHWNDs->push_back(CurrentWindow);
                FillData->Offsets->push_back(CurrentOffset);
                GetRgnResult = CombineRgn(FillData->AvailableRegion, FillData->AvailableRegion, CurrentRegion, RGN_DIFF);
                if (GetRgnResult == NULLREGION || GetRgnResult == ERROR) return FALSE;
            }
        }
    }
    return TRUE;
}