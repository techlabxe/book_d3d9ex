﻿#pragma once
#include <d3d9.h>

#include <DirectXMath.h>
#include <DirectXPackedVector.h>


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
    bool SetupBuffers();
    bool SetupVertexDeclaration();
    bool LoadShader();

    struct MyVertex
    {
        DirectX::XMFLOAT3 Pos;
        DirectX::XMFLOAT2 UV;
    };

    IDirect3D9Ex*   m_d3d9;
    IDirect3DDevice9Ex* m_d3dDev;
    D3DPRESENT_PARAMETERS m_d3dpp;

    IDirect3DVertexDeclaration9* m_Declaration;
    IDirect3DVertexBuffer9* m_VertexBuffer;
    IDirect3DIndexBuffer9* m_IndexBuffer;
    IDirect3DVertexShader9* m_VertexShader;
    IDirect3DPixelShader9*  m_PixelShader;
    IDirect3DTexture9*  m_Texture;

    DirectX::XMMATRIX m_mtxView; // ビュー行列.
    DirectX::XMMATRIX m_mtxProj; // プロジェクション行列.
    int m_VertexCount;
    int m_IndexCount;
};
