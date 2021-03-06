﻿#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <tchar.h>
#include "App.h"

#include <DirectXMath.h>

namespace
{
    // ウィンドウ作成で使用するウィンドウクラス名.
    LPCTSTR szWindowClass = _T("SampleDX9Ex");

    // ウィンドウのタイトルで使用する名前 (アプリケーション名).
    LPCTSTR szTitle = _T("DeferredDX9");

    // ウィンドウのサイズ.
    const int WindowWidth = 1280;
    const int WindowHeight = 720;
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

// 仮想フルスクリーンへの解像度変更
bool EnterVirtualFullScreen(int width, int height)
{
    DEVMODE devMode;

    int nModeIndex = 0;
    while (EnumDisplaySettings(NULL, nModeIndex++, &devMode))
    {
        // 一致する解像度か？
        if (devMode.dmPelsWidth != width || devMode.dmPelsHeight != height)
        {
            continue;
        }
        // ARGB  32bit が使えるか？
        if (devMode.dmBitsPerPel != 32)
        {
            continue;
        }
        // リフレッシュレート
        if (devMode.dmDisplayFrequency != 60)
        {
            continue;
        }
        if (ChangeDisplaySettings(&devMode, CDS_TEST) == DISP_CHANGE_SUCCESSFUL)
        {
            // 切り替えできそうなのでこれを採用する.
            ChangeDisplaySettings(&devMode, CDS_FULLSCREEN);
            return true;
        }
    }
    return false;
}

// 仮想フルスクリーンへの解像度変更解除
void LeaveVirtualFullScreen()
{
    ChangeDisplaySettings(NULL, 0);
}

HWND SetupWindow(HINSTANCE hInstance, int width, int height)
{
    // ウィンドウサイズの変更は禁止とする.
    DWORD dwStyle = WS_OVERLAPPEDWINDOW & ~(WS_SIZEBOX | WS_MAXIMIZEBOX);
    // 作成するウィンドウの大きさの補正をする.
    RECT rect = { 0, 0, WindowWidth, WindowHeight };
    AdjustWindowRect(&rect, dwStyle, FALSE);

    // ウィンドウの作成と表示.
    HWND hWnd = CreateWindow(
        szWindowClass,
        szTitle,
        dwStyle,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
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
    App::ScreenMode screenMode = App::WindowMode;

    // ウィンドウの作成.
    if (App::VirtualFullScreenMode == screenMode)
    {
        EnterVirtualFullScreen(WindowWidth, WindowHeight);
    }
    HWND hWnd = SetupWindow(hInstance, WindowWidth, WindowHeight);
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

    // 仮想フルスクリーンモードの解除.
    if (App::VirtualFullScreenMode == screenMode)
    {
        LeaveVirtualFullScreen();
    }

    return static_cast<int>(msg.wParam);
}
