#include "App.h"
#include <stdexcept>

// D3D9 ライブラリのリンク.
#pragma comment(lib, "d3d9.lib")

App::App()
: m_d3d9(nullptr), m_d3dDev(nullptr)
{
    ZeroMemory(&m_d3dpp, sizeof(m_d3dpp));
}

App::~App()
{
}

bool App::Initialize(HWND hWnd, int width, int height, ScreenMode mode)
{
    try
    {
        HRESULT hr;
        hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &m_d3d9);
        if (FAILED(hr))
            throw std::runtime_error("failed Direct3DCreate9Ex");

        // D3DPRESENT_PARAMETERS のセット.
        m_d3dpp.BackBufferCount = 2;
        m_d3dpp.BackBufferWidth = width;
        m_d3dpp.BackBufferHeight = height;
        m_d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
        m_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        m_d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
        m_d3dpp.EnableAutoDepthStencil = TRUE;
        m_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
        m_d3dpp.hDeviceWindow = hWnd;

        // ディスプレイモード.
        D3DDISPLAYMODEEX dm;
        ZeroMemory(&dm, sizeof(dm));
        dm.Size = sizeof(dm);
        dm.Format = m_d3dpp.BackBufferFormat;
        dm.Width = m_d3dpp.BackBufferWidth;
        dm.Height = m_d3dpp.BackBufferHeight;
        dm.ScanLineOrdering = D3DSCANLINEORDERING_UNKNOWN;

        // 描画デバイスの作成.
        DWORD flag = D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE;

        switch (mode)
        {
        case App::WindowMode:
            // ウィンドウモード.
            m_d3dpp.Windowed = TRUE;
            hr = m_d3d9->CreateDeviceEx(
                D3DADAPTER_DEFAULT,
                D3DDEVTYPE_HAL,
                hWnd,
                flag,
                &m_d3dpp,
                nullptr,    // 指定しない！
                &m_d3dDev);
            break;

        case App::FullScreenMode:
            // フルスクリーンモード.
            // リフレッシュレートは 60Hz を期待.
            m_d3dpp.Windowed = FALSE;
            m_d3dpp.FullScreen_RefreshRateInHz = 60;
            dm.RefreshRate = m_d3dpp.FullScreen_RefreshRateInHz;
            hr = m_d3d9->CreateDeviceEx(
                D3DADAPTER_DEFAULT,
                D3DDEVTYPE_HAL,
                hWnd,
                flag,
                &m_d3dpp,
                &dm,    // 指定する！
                &m_d3dDev);
            break;

        default:
            throw std::runtime_error("Not found ScreenType");
        }
        if (FAILED(hr))
            throw std::runtime_error("failed CreateDeviceEx");
    }
    catch (std::runtime_error e)
    {
        MessageBoxA(hWnd, e.what(), "初期化失敗", MB_OK);
        return false;
    }
    return true;
}

void App::Render()
{
    // 画面を塗りつぶす.
    DWORD dwClearFlags = D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL;
    DWORD dwClearColor = D3DCOLOR_RGBA(0x40, 0x80, 0xFF, 0x00);
    m_d3dDev->Clear(0, nullptr, dwClearFlags, dwClearColor, 1.0f, 0);
    m_d3dDev->PresentEx(nullptr, nullptr, nullptr, nullptr, 0);
}

void App::Terminate()
{
    SafeRelease(m_d3dDev);
    SafeRelease(m_d3d9);
}
