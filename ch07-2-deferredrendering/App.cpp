#include "App.h"
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "TeapotModel.h"

// D3D9 ライブラリのリンク.
#pragma comment(lib, "d3d9.lib")

using namespace DirectX;
using namespace DirectX::PackedVector;

namespace
{
// 実行体のあるファイルパスを返却する.
std::wstring GetExecutionDirectory()
{
    wchar_t filePath[MAX_PATH];
    GetModuleFileNameW(NULL, filePath, sizeof(filePath));
    wchar_t* p = wcsrchr(filePath, L'\\');

    std::wstring strPath(filePath, p);
    return strPath;
}

bool LoadBinaryFile(const std::wstring& fileName, std::vector<uint8_t>& loadBuf)
{
    loadBuf.clear();
    std::wstring path = GetExecutionDirectory();
    path += std::wstring(L"\\");
    path += fileName;

    std::ifstream infile(path, std::ifstream::binary);
    if (!infile)
    {
        return false;
    }

    int size = static_cast<int>(infile.seekg(0, std::ifstream::end).tellg());
    loadBuf.resize(size);
    infile.seekg(0, std::ifstream::beg);
    infile.read(reinterpret_cast<char*>(loadBuf.data()), size);
    return true;
}

template<class T>
void CopyToBuffer(T* resource, const void* src, size_t length)
{
    void* dst;
    HRESULT hr = resource->Lock(0, 0, &dst, 0);
    if (SUCCEEDED(hr))
    {
        memcpy_s(dst, length, src, length);
        resource->Unlock();
    }
    if (FAILED(hr))
        throw std::runtime_error("Failed Lock");
}

}


App::App()
    : m_d3d9(nullptr), m_d3dDev(nullptr),
    m_DeclarationPT(nullptr), m_DeclarationPN(nullptr),
    m_renderWorldPos(nullptr),
    m_renderWorldNormal(nullptr),
    m_renderDiffuse(nullptr)

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
        case App::VirtualFullScreenMode:
            // ウィンドウモード もしくは 仮想フルスクリーンモード.
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

        SetupVertexDeclarations();
        SetupBuffers();
        LoadShader();

        SetupGBuffers(width, height);

        // ビュー行列とプロジェクション行列をセットアップ.

        // カメラ（視点）の情報.
        XMFLOAT3 eyePos(0.0f, 4.0f, -10.0f);
        XMFLOAT3 eyeTarget(0.0f, 0.0f, 0.0f);
        XMFLOAT3 eyeUp(0.0f, 1.0f, 0.0f);
        m_mtxView = XMMatrixLookAtLH(
            XMLoadFloat3(&eyePos),
            XMLoadFloat3(&eyeTarget),
            XMLoadFloat3(&eyeUp)
        );

        // プロジェクション行列.
        float fov = XMConvertToRadians(45.f);
        float aspect = float(width) / float(height);
        float nearZ = 0.1f;
        float farZ = 100.0f;
        m_mtxProj = XMMatrixPerspectiveFovLH(
            fov,
            aspect,
            nearZ,
            farZ
        );

        // ビューポートの設定.
        D3DVIEWPORT9 vp;
        vp.X = 0;
        vp.Y = 0;
        vp.Width = width;
        vp.Height = height;
        vp.MinZ = 0.0f;
        vp.MaxZ = 1.0f;
        m_d3dDev->SetViewport(&vp);


  

        {
            size_t lengthVB = sizeof(TeapotModel::TeapotVerticesPN);
            size_t lengthIB = sizeof(TeapotModel::TeapotIndices);
            m_teapot.indexCount = lengthIB / sizeof(TeapotModel::TeapotIndices[0]);
            m_teapot.vertexCount = lengthVB / sizeof(TeapotModel::Vertex);
            m_d3dDev->CreateVertexBuffer(lengthVB, 0, 0, D3DPOOL_DEFAULT, &m_teapot.vb, nullptr);
            m_d3dDev->CreateIndexBuffer(lengthIB, 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &m_teapot.ib, nullptr);

            CopyToBuffer(m_teapot.vb, TeapotModel::TeapotVerticesPN, lengthVB);
            CopyToBuffer(m_teapot.ib, TeapotModel::TeapotIndices, lengthIB);
        }
    }
    catch (std::runtime_error e)
    {
        MessageBoxA(hWnd, e.what(), "初期化失敗", MB_OK);
        return false;
    }

    return true;
}

void App::SetupGBuffers(int width, int height)
{
    // ワールド位置出力用.
    IDirect3DTexture9* renderTexture;
    renderTexture = CreateDeferredTarget(width, height, D3DFMT_A32B32G32R32F);
    if (!renderTexture)
        throw std::runtime_error("Failed CreateDeferredTarget(worldPos)");

    m_renderWorldPos = new RenderTarget(m_d3dDev, renderTexture);
    renderTexture->Release();

    // 法線出力用.
    renderTexture = CreateDeferredTarget(width, height, D3DFMT_A16B16G16R16F);
    if (!renderTexture)
        throw std::runtime_error("Failed CreateDeferredTarget(worldNormal)");

    m_renderWorldNormal = new RenderTarget(m_d3dDev, renderTexture);
    renderTexture->Release();

    // Diffuse 出力用.
    renderTexture = CreateDeferredTarget(width, height, D3DFMT_A8R8G8B8);
    if (!renderTexture)
        throw std::runtime_error("Failed CreateDeferredTarget(diffuse)");

    m_renderDiffuse = new RenderTarget(m_d3dDev, renderTexture);
    renderTexture->Release();
}

IDirect3DTexture9* App::CreateDeferredTarget(int width, int height, D3DFORMAT format)
{
    HRESULT hr;
    IDirect3DTexture9* renderTexture = nullptr;
    hr = m_d3dDev->CreateTexture(
        width, 
        height, 
        1, 
        D3DUSAGE_RENDERTARGET, 
        format,
        D3DPOOL_DEFAULT, 
        &renderTexture, 
        nullptr);
    if (FAILED(hr))
    {
        return nullptr;
    }
    return renderTexture;
}

void App::Render()
{
    IDirect3DSurface9* primaryColor;

    m_d3dDev->GetRenderTarget(0, &primaryColor);

    m_d3dDev->SetRenderTarget(0, m_renderWorldPos->GetTargetSurface());
    m_d3dDev->SetRenderTarget(1, m_renderWorldNormal->GetTargetSurface());
    m_d3dDev->SetRenderTarget(2, m_renderDiffuse->GetTargetSurface());

    // 画面を塗りつぶす.
    DWORD dwClearFlags = D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL;
    DWORD dwClearColor = D3DCOLOR_RGBA(0x40, 0x80, 0xFF, 0x00);
    m_d3dDev->Clear(0, nullptr, dwClearFlags, 0, 1.0f, 0);
    m_d3dDev->BeginScene();

    // ワールド行列は単位行列.
    XMMATRIX world = XMMatrixIdentity();

    // 計算した行列を float で格納しているデータ型へ転置して格納し直す.
    XMFLOAT4X4 mtxWorld, mtxViewProj;
    XMStoreFloat4x4(&mtxWorld, XMMatrixTranspose(world));
    XMStoreFloat4x4(&mtxViewProj, XMMatrixTranspose(m_mtxView * m_mtxProj) );

    // シェーダー定数へ値をセット
    m_d3dDev->SetVertexShaderConstantF(0, &mtxWorld.m[0][0], 4);
    m_d3dDev->SetVertexShaderConstantF(4, &mtxViewProj.m[0][0], 4);

    // シェーダーをセット.
    m_d3dDev->SetVertexShader(m_mapVS[L"Deferred_FirstPass"]);
    m_d3dDev->SetPixelShader(m_mapPS[L"Deferred_FirstPass"]);

    m_d3dDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    m_d3dDev->SetRenderState(D3DRS_ZENABLE, TRUE);

    // モデルを描画する
    XMFLOAT3 modelPos[] = {
        XMFLOAT3(0.0f, 0.85f, 0.0f),
        XMFLOAT3(-3.0f, 0.85f, -2.0f),
        XMFLOAT3(+3.0f, 0.85f, -2.0f),
        XMFLOAT3(-3.0f, 0.85f,  2.0f),
        XMFLOAT3(+3.0f, 0.85f,  2.0f),
    };
    XMFLOAT4 teapotColor[] = {
        XMFLOAT4(0.8f, 1.0f,0.8f, 1.0f),
        XMFLOAT4(0.8f, 0.7f,0.6f, 1.0f),
        XMFLOAT4(0.3f, 0.5f,0.4f, 1.0f),
        XMFLOAT4(0.3f, 0.5f,0.7f, 1.0f),
        XMFLOAT4(0.8f, 0.8f,0.8f, 1.0f),
    };
    for (int i = 0; i < _countof(modelPos); ++i)
    {
        XMFLOAT4X4 world;
        XMStoreFloat4x4( 
            &world, 
            XMMatrixTranslation(modelPos[i].x, modelPos[i].y, modelPos[i].z)
            );
        DrawModel(m_teapot, world, teapotColor[i]);
    }

    m_d3dDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    XMFLOAT4X4 identity;
    XMStoreFloat4x4(&identity, XMMatrixIdentity());
    DrawModel(m_floor, identity, XMFLOAT4(1,1,1,1));

#if 01
    // プライマリのバッファに戻す.
    m_d3dDev->SetRenderTarget(0, primaryColor);
    m_d3dDev->SetRenderTarget(1, nullptr);
    m_d3dDev->SetRenderTarget(2, nullptr);
    m_d3dDev->SetRenderTarget(3, nullptr);
    m_d3dDev->Clear(D3DADAPTER_DEFAULT, NULL, dwClearFlags, D3DCOLOR_XRGB(0,192,64), 1.0f, 0);

    m_d3dDev->SetVertexDeclaration(m_DeclarationPT);

    m_d3dDev->SetVertexShader(m_mapVS[L"Deferred_LightingPass"]);
    m_d3dDev->SetPixelShader(m_mapPS[L"Deferred_LightingPass"]);

    m_d3dDev->SetTexture(0, m_renderWorldPos->GetTexture());
    m_d3dDev->SetTexture(1, m_renderWorldNormal->GetTexture());
    m_d3dDev->SetTexture(2, m_renderDiffuse->GetTexture());

    m_d3dDev->SetRenderState(D3DRS_ZENABLE, FALSE);

    struct LightInfo
    {
        XMFLOAT4 Pos;
        XMFLOAT4 Color;
    };

    LightInfo lightInfo[] = {
        { XMFLOAT4(0.0f, 8.0f,-4.0f, 10.0f), XMFLOAT4(1.0f,1.0f,1.0f,1) }, // Amb
        // 各 teapot 照らし用
        { XMFLOAT4( 0.0f, 2.3f,-1.0f, 2.0f), XMFLOAT4(1.0f,1.0f,1.0f,1) },
        { XMFLOAT4(-3.5f, 2.3f,-3.0f, 2.0f), XMFLOAT4(0.6f,0.1f,1.0f,1) },
        { XMFLOAT4(+3.5f, 2.3f,-3.0f, 2.0f), XMFLOAT4(0.0f,1.0f,0.0f,1) },
        { XMFLOAT4(-3.5f, 2.3f,+1.0f, 2.0f), XMFLOAT4(0.0f,0.0f,1.0f,1) },
        { XMFLOAT4(+3.5f, 2.3f,+1.0f, 2.0f), XMFLOAT4(1.0f,0.0f,0.0f,1) },

        { XMFLOAT4(-1.5f, 1.0f, -3.5f, 3.0f), XMFLOAT4(0.0f,0.8f,0.0f,1) },
        { XMFLOAT4(+1.5f, 1.0f, -3.5f, 3.0f), XMFLOAT4(0.3f,0.6f,1.0f,1) },
        { XMFLOAT4(-1.5f, 1.0f,  0.5f, 3.0f), XMFLOAT4(1.0f,0.9f,0.1f,1) },
        { XMFLOAT4(+1.5f, 1.0f,  0.5f, 3.0f), XMFLOAT4(1.0f,0.4f,0.9f,1) },

        { XMFLOAT4( 0.0f, 1.5, -1.5f, 2.0f), XMFLOAT4(0.3f,0.1f,0.9f,1) }, 
        { XMFLOAT4(-2.5f, 3.0f,  -.5f, 4.0f), XMFLOAT4(0.3f,1.0f,0.9f,1) },
        { XMFLOAT4( 2.75f,0.85f, 1.0f, 3.0f), XMFLOAT4(0.3f,1.0f,0.9f,1) },

        { XMFLOAT4(-3.0f, 1.5f,-4.0f, 2.5f), XMFLOAT4(0.3f,0.8f,1.0f,1) },
        { XMFLOAT4(+3.0f, 1.5f,-4.0f, 2.5f), XMFLOAT4(0.8f,0.1f,1.0f,1) },
        { XMFLOAT4(+2.5f, 0.5f, 1.0f, 2.5f), XMFLOAT4(1.0f,0.7f,0.1f,1) },

    };
    const int lightCount = _countof(lightInfo);
    m_d3dDev->SetPixelShaderConstantF(0, (float*)(&lightInfo[0]), lightCount*2);

    struct VertexPT
    {
        XMFLOAT3 Pos;
        XMFLOAT2 UV;
    } verticesFullScreenQuad[] = {
        { XMFLOAT3(-1.0f, 1.0f,0.0f), XMFLOAT2(0.0f,0.0f) },
        { XMFLOAT3( 1.0f, 1.0f,0.0f), XMFLOAT2(1.0f,0.0f) },
        { XMFLOAT3(-1.0f,-1.0f,0.0f), XMFLOAT2(0.0f,1.0f) },
        { XMFLOAT3( 1.0f,-1.0f,0.0f), XMFLOAT2(1.0f,1.0f) },
    };
    
    m_d3dDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, verticesFullScreenQuad, sizeof(VertexPT) );
#endif
    m_d3dDev->EndScene();
    primaryColor->Release();

    HRESULT hr;
    hr = m_d3dDev->PresentEx(nullptr, nullptr, nullptr, nullptr, 0);
    if (FAILED(hr))
    {
        if (hr == D3DERR_DEVICEREMOVED)
        {
            OutputDebugStringA("D3DERR_DEVICEREMOVED\n");
            hr = m_d3dDev->ResetEx(&m_d3dpp, nullptr);
        }
        if (hr == D3DERR_DEVICEHUNG)
        {
            OutputDebugStringA("D3DERR_DEVICEHUNG\n");

        }
        if (hr == D3DERR_DEVICELOST)
        {
            OutputDebugStringA("DeviceLost\n");
        }
    }
}

void App::Terminate()
{
    delete m_renderWorldPos;
    delete m_renderWorldNormal;
    delete m_renderDiffuse;
    m_renderWorldPos = nullptr;
    m_renderWorldNormal = nullptr;
    m_renderDiffuse = nullptr;

    SafeRelease(m_DeclarationPN);
    SafeRelease(m_DeclarationPT);
    SafeRelease(m_d3dDev);
    SafeRelease(m_d3d9);
}

void App::DrawModel(const Model& model, const DirectX::XMFLOAT4X4& world, const DirectX::XMFLOAT4& color)
{
    m_d3dDev->SetVertexDeclaration(m_DeclarationPN);
    m_d3dDev->SetStreamSource(0, model.vb, 0, sizeof(MyVertexPN));
    m_d3dDev->SetIndices(model.ib);

    XMFLOAT4X4 transposed;
    XMStoreFloat4x4(
        &transposed,
        XMMatrixTranspose(XMLoadFloat4x4(&world))
    );

    m_d3dDev->SetVertexShaderConstantF(0, &transposed.m[0][0], 4);
    m_d3dDev->SetVertexShaderConstantF(8, &color.x, 1);

    m_d3dDev->DrawIndexedPrimitive(
        D3DPT_TRIANGLELIST, 
        0, 
        0, 
        model.vertexCount, 
        0, 
        model.indexCount / 3);

}

// 頂点宣言の作成・準備を行います.
void App::SetupVertexDeclarations()
{
    D3DVERTEXELEMENT9 declsPT[] = {
        // Stream, Offset, Type, Method, Usage, UsageIndex
        { 0,  0, D3DDECLTYPE_FLOAT3,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 12, D3DDECLTYPE_FLOAT2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        D3DDECL_END(),
    };
    D3DVERTEXELEMENT9 declsPN[] = {
        // Stream, Offset, Type, Method, Usage, UsageIndex
        { 0,  0, D3DDECLTYPE_FLOAT3,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 12, D3DDECLTYPE_FLOAT3,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
        D3DDECL_END(),
    };
    
    if (FAILED(m_d3dDev->CreateVertexDeclaration(declsPT, &m_DeclarationPT)))
        throw std::runtime_error("Failed CreateVertexDeclaration");

    if (FAILED(m_d3dDev->CreateVertexDeclaration(declsPN, &m_DeclarationPN)))
        throw std::runtime_error("Failed CreateVertexDeclaration");
}

// 頂点バッファ・インデックスバッファの作成・準備を行います.
void App::SetupBuffers()
{
    MyVertexPN floorVertices[] = {
        { XMFLOAT3(-5,0, 5), XMFLOAT3(0,1,0) },
        { XMFLOAT3(5,0, 5), XMFLOAT3(0,1,0) },
        { XMFLOAT3(-5,0,-5), XMFLOAT3(0,1,0) },
        { XMFLOAT3(5,0,-5), XMFLOAT3(0,1,0) },
    };
    uint16_t floorIndices[] = {
        0, 1, 2,
        2, 1, 3,
    };

    UINT lengthVB = sizeof(floorVertices);
    UINT lengthIB = sizeof(floorIndices);
    m_floor.vertexCount = lengthVB / sizeof(floorVertices[0]);
    m_floor.indexCount = lengthIB / sizeof(floorIndices[0]);

    HRESULT hr;
    hr = m_d3dDev->CreateVertexBuffer(
        lengthVB,
        0,
        0,
        D3DPOOL_DEFAULT,
        &m_floor.vb,
        nullptr);
    if (FAILED(hr))
        throw std::runtime_error("Failed CreateVertexBuffer");

    hr = m_d3dDev->CreateIndexBuffer(
        lengthIB,
        0,
        D3DFMT_INDEX16,
        D3DPOOL_DEFAULT,
        &m_floor.ib,
        nullptr);
    if (FAILED(hr))
        throw std::runtime_error("Failed CreateIndexBuffer");

    // 事前に用意した頂点データをコピーする.
    CopyToBuffer(m_floor.vb, floorVertices, lengthVB);

    // 事前に用意したインデックスデータをコピーする.
    CopyToBuffer(m_floor.ib, floorIndices, lengthIB);


}

void App::LoadShader()
{
    HRESULT hr;
    std::vector<uint8_t> buf;

    wchar_t fileName[128];
    const wchar_t* shaderPass[] = {
        L"Deferred_FirstPass",
        L"Deferred_LightingPass",
    };
    const int count = _countof(shaderPass);
    for (int i = 0; i < count; ++i)
    {
        for (int type = 0; type < 2; ++type)
        {
            const wchar_t* shaderType = type == 0 ? L"VS" : L"PS";
            wsprintf(fileName, L"%s_%s.cso", shaderPass[i], shaderType);

            if (!LoadBinaryFile(fileName, buf))
                throw std::runtime_error("Failed load shader file");

            DWORD* code = reinterpret_cast<DWORD*>(buf.data());
            size_t size = buf.size();
            if (type == 0)
            {
                IDirect3DVertexShader9* vs;
                hr = m_d3dDev->CreateVertexShader(code, &vs);
                if (vs)
                {
                    m_mapVS[shaderPass[i]] = vs;
                }
            }
            else
            {
                IDirect3DPixelShader9* ps;
                hr = m_d3dDev->CreatePixelShader(code, &ps);
                if (ps)
                {
                    m_mapPS[shaderPass[i]] = ps;
                }
            }
            if (FAILED(hr))
                throw std::runtime_error("Failed CreateVertex/PixelShader");
        }
    }
}


RenderTarget::RenderTarget(IDirect3DDevice9Ex* d3dDev, IDirect3DTexture9* texture)
    : m_d3dDev(d3dDev), m_texture(texture)
{
    if (m_d3dDev)
    {
        m_d3dDev->AddRef();
    }
    if (m_texture)
    {
        m_d3dDev->AddRef();
    }

    m_texture->GetSurfaceLevel(0, &m_surface);
}
RenderTarget::~RenderTarget()
{
    if (m_d3dDev)
    {
        m_d3dDev->Release();
    }
    if (m_surface)
    {
        m_surface->Release();
    }
    if (m_texture)
    {
        m_texture->Release();
    }
    m_d3dDev = nullptr;
    m_texture = nullptr;
    m_surface = nullptr;
}

