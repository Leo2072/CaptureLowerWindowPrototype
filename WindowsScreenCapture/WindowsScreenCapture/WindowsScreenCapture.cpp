#include "EnumWindowsProcs.h"
#include <magnification.h>


#pragma comment(lib, "Magnification.lib")


LRESULT CALLBACK WndProc(HWND Window, UINT message, WPARAM wParam, LPARAM lParam);


HWND hwnd, Mag;
HBRUSH BackgroundBrush;
bool UseEnumWindows = true;
unsigned short BorderHitType = NULL;
POINTS MouseOffset;


void RenderLoop(HWND Window, HWND MagWindow);
RECT GetBorderOffset(HWND Window);


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    if (!MagInitialize()) return -1;

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
    hwnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED, ClassName, L"Screen Inverter", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, 0, 0, 600, 600, NULL, NULL, hInstance, NULL);
    Mag = CreateWindow(WC_MAGNIFIER, L"Magnifier Window", WS_CHILD | WS_VISIBLE, 0, 0, 600, 600, hwnd, NULL, hInstance, NULL);


    MAGTRANSFORM TransformMatrix{ 0 };
    TransformMatrix.v[0][0] = 1;
    TransformMatrix.v[1][1] = 1;
    TransformMatrix.v[2][2] = 1;
    MagSetWindowTransform(Mag, &TransformMatrix);

    MAGCOLOREFFECT ColorTransformMatrix
    {{
        {-1, 0, 0, 0, 0},
        {0, -1, 0, 0, 0},
        {0, 0, -1, 0, 0},
        {0, 0, 0, 1, 0},
        {1, 1, 1, 1, 1}
    }};
    MagSetColorEffect(Mag, &ColorTransformMatrix);


    //SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE);
    SetLayeredWindowAttributes(hwnd, RGB(255, 255, 255), 255, LWA_COLORKEY);
    ShowWindow(hwnd, SW_MAXIMIZE);

    MSG msg;

    // Main message loop:
    bool Running = true;

    long MinRenderDelta = 40;
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
            RenderLoop(hwnd, Mag);
            TimeSinceLastRender = 0;
        }
        PreviousTime = CurrentTime;
    }

    DeleteObject(BackgroundBrush);

    MagUninitialize();

    return (int) msg.wParam;
}


LRESULT CALLBACK WndProc(HWND Window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_NCLBUTTONDOWN:
    {
        switch (wParam)
        {
        case HTLEFT:
        case HTRIGHT:
        case HTTOP:
        case HTTOPLEFT:
        case HTTOPRIGHT:
        case HTBOTTOM:
        case HTBOTTOMLEFT:
        case HTBOTTOMRIGHT:
        case HTCAPTION:
        {
            SetActiveWindow(Window);
            BorderHitType = wParam;
            RECT WindowRect;
            GetWindowRect(Window, &WindowRect);
            POINTS NewMouseOffset(MAKEPOINTS(lParam));
            NewMouseOffset.x -= WindowRect.left;
            NewMouseOffset.y -= WindowRect.top;
            MouseOffset = NewMouseOffset;
            SetCapture(Window);
        }
        break;
        default:
            DefWindowProc(Window, message, wParam, lParam);
        break;
        }
    }
    break;
    case WM_LBUTTONUP:
    {
        BorderHitType = NULL;
        ReleaseCapture();
    }
    break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(Window, message, wParam, lParam);
    }
    return 0;
}


void RenderLoop(HWND Window, HWND MagWindow)
{
    if (BorderHitType)
    {
        RECT WindowRect;
        GetWindowRect(Window, &WindowRect);
        POINT CursorPos;
        GetCursorPos(&CursorPos);
        switch(BorderHitType)
        {
        case HTLEFT:
            SetWindowPos(Window, 0, CursorPos.x, WindowRect.top, WindowRect.right - CursorPos.x, WindowRect.bottom - WindowRect.top, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOSENDCHANGING);
            break;
        case HTRIGHT:
            SetWindowPos(Window, 0, 0, 0, CursorPos.x - WindowRect.left, WindowRect.bottom - WindowRect.top, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOSENDCHANGING);
            break;
        case HTTOP:
            SetWindowPos(Window, 0, WindowRect.left, CursorPos.y, WindowRect.right - WindowRect.left, WindowRect.bottom - CursorPos.y, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOSENDCHANGING);
            break;
        case HTTOPLEFT:
            SetWindowPos(Window, 0, CursorPos.x, CursorPos.y, WindowRect.right - CursorPos.x, WindowRect.bottom - CursorPos.y, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOSENDCHANGING);
            break;
        case HTTOPRIGHT:
            SetWindowPos(Window, 0, WindowRect.left, WindowRect.top, CursorPos.x - WindowRect.left, WindowRect.bottom - CursorPos.y, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOSENDCHANGING);
            break;
        case HTBOTTOM:
            SetWindowPos(Window, 0, 0, 0, WindowRect.right - WindowRect.left, CursorPos.y - WindowRect.top, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOSENDCHANGING);
            break;
        case HTBOTTOMLEFT:
            SetWindowPos(Window, 0, CursorPos.x, WindowRect.top, WindowRect.right - CursorPos.x, CursorPos.y - WindowRect.top, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOSENDCHANGING);
            break;
        case HTBOTTOMRIGHT:
            SetWindowPos(Window, 0, 0, 0, CursorPos.x - WindowRect.left, CursorPos.y - WindowRect.top, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOSENDCHANGING);
            break;
        case HTCAPTION:
            SetWindowPos(Window, 0, CursorPos.x - MouseOffset.x, CursorPos.y - MouseOffset.y, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOSENDCHANGING);
            break;
        }
    }
    RECT SourceRect{ 0 };
    POINT Offset{ 0 };
    GetClientRect(Window, &SourceRect);
    SetWindowPos(MagWindow, 0, 0, 0, SourceRect.right, SourceRect.bottom, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOZORDER | SWP_NOSENDCHANGING);
    ClientToScreen(Window, &Offset);

    SourceRect.left += Offset.x; SourceRect.right += Offset.x;
    SourceRect.top += Offset.y; SourceRect.bottom += Offset.y;

    MagSetWindowSource(MagWindow, SourceRect);
    InvalidateRect(MagWindow, NULL, TRUE);
}


RECT GetBorderOffset(HWND Window)
{
    RECT WindowRect;
    RECT ClientRect;
    GetWindowRect(Window, &WindowRect);
    GetClientRect(Window, &ClientRect);
    POINT ClientLT{ ClientRect.left, ClientRect.top };
    POINT ClientRB{ ClientRect.right, ClientRect.bottom };
    ClientToScreen(Window, &ClientLT);
    ClientToScreen(Window, &ClientRB);

    return RECT({ ClientLT.x - WindowRect.left, ClientRB.x - WindowRect.right, ClientLT.y - WindowRect.top, ClientRB.y - WindowRect.bottom });
}