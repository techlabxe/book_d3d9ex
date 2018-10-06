#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <tchar.h>
#include "App.h"

namespace
{
    // ウィンドウ作成で使用するウィンドウクラス名.
    LPCTSTR szWindowClass = _T("SampleDX9Ex");

    // ウィンドウのタイトルで使用する名前 (アプリケーション名).
    LPCTSTR szTitle = _T("HelloDX9FullScreen");

    // ウィンドウのサイズ.
    const int WindowWidth = 800;
    const int WindowHeight = 600;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    switch (msg)
    {
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            PostMessage(hWnd, WM_CLOSE, 0, 0);
        }
        break;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

HWND SetupWindow(HINSTANCE hInstance, int width, int height, App::ScreenMode mode)
{
    DWORD dwStyle;
    RECT rect{ 0, 0, WindowWidth, WindowHeight };
    int cx = CW_USEDEFAULT, cy = CW_USEDEFAULT;

    switch (mode)
    {
    case App::WindowMode:
        // ウィンドウモード.
        // ウィンドウサイズの変更は禁止とする.
        dwStyle = WS_OVERLAPPEDWINDOW & ~(WS_SIZEBOX | WS_MAXIMIZEBOX);

        // 作成するウィンドウの大きさの補正をする.
        AdjustWindowRect(&rect, dwStyle, FALSE);
        break;

    case App::FullScreenMode:
        // フルスクリーンモード.
        // 枠やタイトルバーはなしで作成し、サイズの補正も必要なし.
        dwStyle = WS_POPUP;
        cx = 0;
        cy = 0;
        break;
    default:
        break;
    }

    // ウィンドウの作成と表示.
    HWND hWnd = CreateWindow(
        szWindowClass,
        szTitle,
        dwStyle,
        cx,
        cy,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        nullptr,
        hInstance,
        nullptr);
    return hWnd;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow)
{
    // ウィンドウクラスの準備.
    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
    wc.lpszClassName = szWindowClass;
    wc.lpfnWndProc = WndProc;
    if (!RegisterClassEx(&wc))
    {
        // ウィンドウクラスの登録で失敗.
        return -1;
    }

    // モードの選択.
    App::ScreenMode screenMode = App::FullScreenMode;

    // ウィンドウの作成.
    HWND hWnd = SetupWindow(hInstance, WindowWidth, WindowHeight, screenMode);
    if (!hWnd)
    {
        // ウィンドウ作成で失敗.
        return -1;
    }
    UpdateWindow(hWnd);
    ShowWindow(hWnd, nCmdShow);

    // DirectX の初期化処理.
    App app;
    app.Initialize(hWnd, WindowWidth, WindowHeight, screenMode);

    // Windows のメッセージループを回す.
    bool finished = false;
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    do
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
            {
                finished = true;
                break;
            }
        }
        if (!finished)
        {
            // DirectX による描画.
            app.Render();
        }
    } while (!finished);

    // DirectX の終了処理.
    app.Terminate();

    return static_cast<int>(msg.wParam);
}
