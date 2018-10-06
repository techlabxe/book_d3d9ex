#pragma once
#include <d3d9.h>

#include <DirectXMath.h>
#include <DirectXPackedVector.h>


class App
{
public:
    App();
    ~App();

    // 動作モード.
    enum ScreenMode {
        WindowMode,
        FullScreenMode,
        VirtualFullScreenMode,
    };

    bool Initialize(HWND hWnd, int width, int height, ScreenMode mode);
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
    void SetupVertexBuffer();
    void SetupIndexBuffer();
    void SetupVertexDeclaration();
    void LoadShader();

    IDirect3D9Ex*   m_d3d9;
    IDirect3DDevice9Ex* m_d3dDev;
    D3DPRESENT_PARAMETERS m_d3dpp;

    IDirect3DVertexDeclaration9* m_Declaration;
    IDirect3DVertexBuffer9* m_VertexBuffer;
    IDirect3DIndexBuffer9* m_IndexBuffer;
    IDirect3DVertexShader9* m_VertexShader;
    IDirect3DPixelShader9*  m_PixelShader;

    DirectX::XMMATRIX m_mtxView; // ビュー行列.
    DirectX::XMMATRIX m_mtxProj; // プロジェクション行列.
};
