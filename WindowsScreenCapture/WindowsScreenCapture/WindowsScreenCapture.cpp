#include "EnumWindowsProcs.h"


LRESULT CALLBACK WndProc(HWND Window, UINT message, WPARAM wParam, LPARAM lParam);


HWND hwnd;
HBRUSH BackgroundBrush;
bool UseEnumWindows = true;


void RgnBitBlt(HDC target, long tOffsetX, long tOffsetY, HDC src, long sOffsetX, long sOffsetY, HRGN rgn, DWORD rop);

void CaptureBelowWindow(HWND CurrentWindow, HDC hdc, DWORD rop);


void RenderLoop(HWND Window);


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    BackgroundBrush = CreateSolidBrush(RGB(255, 255, 255));
    const wchar_t ClassName[] = L"Screen Under Capture";
    WNDCLASSEXW wcex{ 0 };

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.lpszClassName = ClassName;
    wcex.hbrBackground = BackgroundBrush;

    RegisterClassEx(&wcex);
    hwnd = CreateWindowEx(WS_EX_TRANSPARENT | WS_EX_LAYERED, ClassName, L"Screen Capture Thing", WS_OVERLAPPEDWINDOW, 0, 0, 1733, 600, NULL, NULL, hInstance, NULL);

    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
    ShowWindow(hwnd, SW_MAXIMIZE);

    MSG msg;

    // Main message loop:
    bool Running = true;

    long MinRenderDelta = 100;
    long TimeSinceLastRender = MinRenderDelta;

    long CurrentTime; long PreviousTime = GetTickCount(); long PreviousFixedUpdateTime = PreviousTime;
    long MilliDelta = 0;

    while (Running)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
            {
                Running = false;
                break;
            }
        }
        CurrentTime = GetTickCount();
        MilliDelta = CurrentTime - PreviousTime;
        TimeSinceLastRender += MilliDelta;

        if (TimeSinceLastRender >= MinRenderDelta)
        {
            SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOREDRAW | SWP_NOMOVE | SWP_NOSIZE | SWP_NOSENDCHANGING);
            RenderLoop(hwnd);
            TimeSinceLastRender = 0;
        }
        PreviousTime = CurrentTime;
    }

    DeleteObject(BackgroundBrush);

    return (int) msg.wParam;
}


LRESULT CALLBACK WndProc(HWND Window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(Window, message, wParam, lParam);
    }
    return 0;
}


void RgnBitBlt(HDC target, long tOffsetX, long tOffsetY, HDC src, long sOffsetX, long sOffsetY, HRGN rgn, DWORD rop)
{
    std::vector<RECT> TransferRects;

    DWORD nSize = GetRegionData(rgn, 0, NULL);

    RGNDATA* RegionData = (RGNDATA*) malloc(nSize);

    GetRegionData(rgn, nSize, RegionData);

    RECT* Buffer = (RECT*) RegionData->Buffer;

    for (DWORD id = 0; id < RegionData->rdh.nCount; id++)
    {
        TransferRects.push_back(Buffer[id]);
    }

    for (RECT& rect : TransferRects)
    {
        BitBlt(target, rect.left - tOffsetX, rect.top - tOffsetY, rect.right - rect.left, rect.bottom - rect.top, src, rect.left - sOffsetX, rect.top - sOffsetY, rop);
    }

    free(RegionData);

    return;
}


void CaptureBelowWindow(HWND CurrentWindow, HDC hdc, DWORD rop)
{
    HWND BottomWindow = GetWindow(CurrentWindow, GW_HWNDLAST);
    RECT CurrentRect;
    POINT TargetOffset;
    POINT CurrentOffset;

    GetWindowRect(CurrentWindow, &CurrentRect);
    HRGN AvailableRegion(CreateRectRgnIndirect(&CurrentRect));
    TargetOffset.x = CurrentRect.left;
    TargetOffset.y = CurrentRect.top;

    std::vector<HRGN> TransferRegions;
    std::vector<HWND> SourceHWNDs;
    std::vector<POINT> Offsets;


    int GetRgnResult;
    unsigned int Count = 0;

    if (UseEnumWindows)
    {
        GetWndRgnFillData FillData;
        FillData.AvailableRegion = AvailableRegion;
        FillData.Offsets = &Offsets;
        FillData.SourceHWNDs = &SourceHWNDs;
        FillData.TransferRegions = &TransferRegions;

        FilterProcData FilterData;
        FilterData.ExcludeMax = true;
        FilterData.HighestHWND = CurrentWindow;
        FilterData.ProcPtr = GetLowerWndRgn;
        FilterData.lParam = (LPARAM)&FillData;

        EnumWindows(FilterEnumWindows, (LPARAM)&FilterData);
        Count = Offsets.size();
    }
    else
    {
        while (CurrentWindow != BottomWindow)
        {
            CurrentWindow = GetWindow(CurrentWindow, GW_HWNDNEXT);
            if (IsWindowVisible(CurrentWindow))
            {
                if (GetWindowRect(CurrentWindow, &CurrentRect) != FALSE)
                {
                    HRGN CurrentRegion(CreateRectRgnIndirect(&CurrentRect));
                    GetRgnResult = CombineRgn(CurrentRegion, AvailableRegion, CurrentRegion, RGN_AND);
                    if (GetRgnResult != ERROR && GetRgnResult != NULLREGION)
                    {
                        Count++;
                        CurrentOffset.x = CurrentRect.left;
                        CurrentOffset.y = CurrentRect.top;
                        TransferRegions.push_back(CurrentRegion);
                        SourceHWNDs.push_back(CurrentWindow);
                        Offsets.push_back(CurrentOffset);
                        GetRgnResult = CombineRgn(AvailableRegion, AvailableRegion, CurrentRegion, RGN_DIFF);
                        if (GetRgnResult == NULLREGION) break;
                        else if (GetRgnResult == ERROR) return;
                    }
                }
            }
        }
    }

    for (unsigned int id = 0; id < Count; id++)
    {
        HDC SourceHDC = GetWindowDC(SourceHWNDs[id]);
        RgnBitBlt(hdc, TargetOffset.x, TargetOffset.y, SourceHDC, Offsets[id].x, Offsets[id].y, TransferRegions[id], rop);
        ReleaseDC(SourceHWNDs[id], SourceHDC);
        DeleteObject(TransferRegions[id]);
    }
    DeleteObject(AvailableRegion);
    return;
}


void RenderLoop(HWND Window)
{
    PAINTSTRUCT ps;
    BeginPaint(Window, &ps);
    HDC hdc = GetWindowDC(Window);
    RECT WindowRect;
    GetWindowRect(Window, &WindowRect);
    FillRect(hdc, &WindowRect, BackgroundBrush);

    CaptureBelowWindow(Window, hdc, SRCINVERT);
    ReleaseDC(Window, hdc);
    EndPaint(Window, &ps);
    return;
}