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