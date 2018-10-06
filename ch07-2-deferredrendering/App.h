#pragma once
#include <d3d9.h>

#include <DirectXMath.h>
#include <DirectXPackedVector.h>

#include <string>
#include <unordered_map>


class RenderTarget
{
public:
    RenderTarget(IDirect3DDevice9Ex* d3dDev, IDirect3DTexture9* texture);
    ~RenderTarget();

    IDirect3DSurface9* GetTargetSurface() { return m_surface; }
    IDirect3DTexture9* GetTexture() { return m_texture; }

private:
    IDirect3DTexture9 * m_texture;
    IDirect3DSurface9 * m_surface;
    IDirect3DDevice9Ex* m_d3dDev;
};

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
    struct Model
    {
        IDirect3DVertexBuffer9* vb;
        IDirect3DIndexBuffer9* ib;

        int indexCount;
        int vertexCount;
    };

    void SetupBuffers();
    void SetupGBuffers(int width, int height);
    void SetupVertexDeclarations();
    void LoadShader();

    void DrawModel(const Model& model, const DirectX::XMFLOAT4X4& world, const DirectX::XMFLOAT4& color);

    struct MyVertex
    {
        DirectX::XMFLOAT3 Pos;
        DirectX::XMFLOAT2 UV;
    };
    struct MyVertexPN
    {
        DirectX::XMFLOAT3 Pos;
        DirectX::XMFLOAT3 Normal;
    };

    IDirect3D9Ex*   m_d3d9;
    IDirect3DDevice9Ex* m_d3dDev;
    D3DPRESENT_PARAMETERS m_d3dpp;

    IDirect3DVertexDeclaration9* m_DeclarationPT;
    IDirect3DVertexDeclaration9* m_DeclarationPN;

    DirectX::XMMATRIX m_mtxView; // ビュー行列.
    DirectX::XMMATRIX m_mtxProj; // プロジェクション行列.

    IDirect3DTexture9* CreateDeferredTarget(int width, int height, D3DFORMAT format);
    RenderTarget* m_renderWorldPos;
    RenderTarget* m_renderWorldNormal;
    RenderTarget* m_renderDiffuse;

    std::unordered_map<std::wstring, IDirect3DVertexShader9*> m_mapVS;
    std::unordered_map<std::wstring, IDirect3DPixelShader9*> m_mapPS;

    Model m_teapot;
    Model m_floor;
};
