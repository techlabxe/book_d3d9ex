#pragma once
#include <d3d9.h>

class App
{
public:
    App();
    ~App();

    bool Initialize(HWND hWnd, int width, int height);
    void Render();
    void Terminate();

private:
    template<class T>
    void SafeRelease(T*& v)
    {
        if (nullptr != v)
        {
            v->Release();
        }
        v = nullptr;
    }
    IDirect3D9Ex*   m_d3d9;
    IDirect3DDevice9Ex* m_d3dDev;
    D3DPRESENT_PARAMETERS m_d3dpp;
};
