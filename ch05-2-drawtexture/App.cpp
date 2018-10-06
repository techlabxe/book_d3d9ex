#include "App.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define  STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

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

bool LoadBinaryFile(const std::wstring& textureFileName, std::vector<uint8_t>& loadBuf)
{
    loadBuf.clear();
    std::wstring path = GetExecutionDirectory();
    path += std::wstring(L"\\");
    path += textureFileName;

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

IDirect3DTexture9* CreateTextureFromFile(
    IDirect3DDevice9* pd3dDev, 
    const std::wstring& textureFileName)
{
    std::vector<uint8_t> buf;
    if (!LoadBinaryFile(textureFileName, buf))
    {
        return nullptr;
    }
    int width = 0, height = 0;
    int request_component = 4;  // R,G,B,A の4コンポーネント.
    int component = 0;
    uint8_t* pLoad = stbi_load_from_memory(
        buf.data(), 
        int(buf.size()), 
        &width, 
        &height, 
        &component, 
        request_component);

    // テクスチャを生成する.
    HRESULT hr;
    IDirect3DTexture9* pTexture = nullptr;
    IDirect3DTexture9* pStaging = nullptr;
    D3DFORMAT format = D3DFMT_A8R8G8B8;
    hr = pd3dDev->CreateTexture(
        width,
        height,
        1,
        0,
        format,
        D3DPOOL_DEFAULT,
        &pTexture,
        nullptr);
    if (FAILED(hr))
        throw std::runtime_error("Failed CreateTexture(default)");

    hr = pd3dDev->CreateTexture(
        width,
        height,
        1,
        0,
        format,
        D3DPOOL_SYSTEMMEM,
        &pStaging,
        nullptr);
    if (FAILED(hr))
        throw std::runtime_error("Failed CreateTexture(sysmem)");

    // RGBA 画像を DirectX9 にあうようにチャンネル入れ替え.
    const int lineBytes = width * sizeof(uint32_t); // 横方向１行のバイト数.
    for (int y = 0; y < height; ++y)
    {
        uint8_t* p = &pLoad[y*lineBytes];
        for (int x = 0; x < width; ++x)
        {
            int idx = 4 * x;
            uint8_t r, g, b, a;

            r = p[idx + 0];
            g = p[idx + 1];
            b = p[idx + 2];
            a = p[idx + 3];

            p[idx + 0] = b;
            p[idx + 1] = g;
            p[idx + 2] = r;
            p[idx + 3] = a;
        }
    }

    D3DLOCKED_RECT locked;
    hr = pStaging->LockRect(0, &locked, nullptr, 0);
    if (FAILED(hr))
        throw std::runtime_error("Failed LockRect");

    // 作業用テクスチャに画像データを書き込む.
    uint8_t* dst = static_cast<uint8_t*>(locked.pBits);
    uint8_t* src = pLoad;
    int y = width;
    while (y > 0)
    {
        memcpy_s(dst, locked.Pitch, src, lineBytes);
        dst += locked.Pitch;
        src += lineBytes;
        y--;
    }
    pStaging->UnlockRect(0);

    // 実テクスチャへ転送する.
    hr = pd3dDev->UpdateTexture(pStaging, pTexture);
        
    // 後始末.
    stbi_image_free(pLoad);
    pStaging->Release();

    return pTexture;
}

}


App::App()
    : m_d3d9(nullptr), m_d3dDev(nullptr),
    m_Declaration(nullptr), m_VertexBuffer(nullptr),
    m_IndexBuffer(nullptr),
    m_VertexShader(nullptr), m_PixelShader(nullptr),
    m_VertexCount(0), m_IndexCount(0)
{
    ZeroMemory(&m_d3dpp, sizeof(m_d3dpp));
}

App::~App()
{
}
std::vector<uint8_t> texSysMem;

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

        // 頂点宣言の準備.
        SetupVertexDeclaration();

        // 頂点バッファの準備.
        SetupBuffers();

        // シェーダーファイルのロードと生成.
        LoadShader();

        // テクスチャのロード
        LoadTexture();

        // ビュー行列とプロジェクション行列をセットアップ.

        // カメラ（視点）の情報.
        XMFLOAT3 eyePos(0.0f, 2.0f, -4.0f);
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
    m_d3dDev->SetVertexShader(m_VertexShader);
    m_d3dDev->SetPixelShader(m_PixelShader);

    // バッファをセット.
    m_d3dDev->SetVertexDeclaration(m_Declaration);
    m_d3dDev->SetStreamSource(0, m_VertexBuffer, 0, sizeof(MyVertex));
    m_d3dDev->SetIndices(m_IndexBuffer);

    // テクスチャをセット.
    m_d3dDev->SetTexture(0, m_Texture);

    // レンダーステートを変更(アルファブレンド有効化,カリング).
    m_d3dDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    m_d3dDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_d3dDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_d3dDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);


    // プリミティブの数は インデックス数 / 3 で求まる.
    int primitiveCount = m_IndexCount / 3;

    // インデックス付き描画を行う.
    m_d3dDev->DrawIndexedPrimitive(
        D3DPT_TRIANGLELIST, 
        0, 
        0, 
        m_VertexCount, 
        0, 
        primitiveCount);

    m_d3dDev->EndScene();
    m_d3dDev->PresentEx(nullptr, nullptr, nullptr, nullptr, 0);
}

void App::Terminate()
{
    SafeRelease(m_Texture);
    SafeRelease(m_VertexBuffer);
    SafeRelease(m_IndexBuffer);
    SafeRelease(m_Declaration);
    SafeRelease(m_VertexShader);
    SafeRelease(m_PixelShader);
    SafeRelease(m_d3dDev);
    SafeRelease(m_d3d9);
}

// 頂点宣言の作成・準備を行います.
void App::SetupVertexDeclaration()
{
    D3DVERTEXELEMENT9 decls[] = {
        // Stream, Offset, Type, Method, Usage, UsageIndex
        { 0,  0, D3DDECLTYPE_FLOAT3,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 12, D3DDECLTYPE_FLOAT2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        D3DDECL_END(),
    };
    if (FAILED(m_d3dDev->CreateVertexDeclaration(decls, &m_Declaration)))
        throw std::runtime_error("Failed CreateVertexDeclaration");
}

// 頂点バッファ・インデックスバッファの作成・準備を行います.
void App::SetupBuffers()
{
    MyVertex vertices[] = {
        { XMFLOAT3(-1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f,0.0f) },
        { XMFLOAT3( 1.0f, 1.0f, 0.0f), XMFLOAT2(1.0f,0.0f) },
        { XMFLOAT3( 1.0f,-1.0f, 0.0f), XMFLOAT2(1.0f,1.0f) },
        { XMFLOAT3(-1.0f,-1.0f, 0.0f), XMFLOAT2(0.0f,1.0f) },
    };
    uint16_t indices[] = {
        0, 1, 2,
        2, 3, 0
    };
    UINT lengthVB = sizeof(vertices);
    UINT lengthIB = sizeof(indices);
    m_VertexCount = sizeof(vertices)/sizeof(vertices[0]);
    m_IndexCount = sizeof(indices)/sizeof(indices[0]);

    HRESULT hr;
    hr = m_d3dDev->CreateVertexBuffer(
        lengthVB,
        0, 
        0, 
        D3DPOOL_DEFAULT, 
        &m_VertexBuffer, 
        nullptr);
    if (FAILED(hr))
        throw std::runtime_error("Failed CreateVertexBuffer");
    hr = m_d3dDev->CreateIndexBuffer(
        lengthIB,
        0,
        D3DFMT_INDEX16,
        D3DPOOL_DEFAULT,
        &m_IndexBuffer,
        nullptr);
    if (FAILED(hr))
        throw std::runtime_error("Failed CreateIndexBuffer");

    void* p = nullptr;
    hr = m_VertexBuffer->Lock(0, 0, &p, 0);
    if (SUCCEEDED(hr))
    {
        // 事前に用意した頂点データをコピーする.
        memcpy_s(p, lengthVB, vertices, lengthVB);
        m_VertexBuffer->Unlock();
    }
    hr = m_IndexBuffer->Lock(0, 0, &p, 0);
    if (SUCCEEDED(hr))
    {
        // 事前に用意したインデックスデータをコピーする.
        memcpy_s(p, lengthIB, indices, lengthIB);
        m_IndexBuffer->Unlock();
    }
}

void App::LoadShader()
{
    HRESULT hr;
    std::vector<uint8_t> buf;

    // Vertex Shader

    if (!LoadBinaryFile(L"VertexShader.cso", buf))
        throw std::runtime_error("Failed load shader file");

    hr = m_d3dDev->CreateVertexShader(
        reinterpret_cast<DWORD*>(buf.data()), 
        &m_VertexShader);
    if (FAILED(hr))
        throw std::runtime_error("Failed CreateVertexShader");

    // Pixel Shader

    if (!LoadBinaryFile(L"PixelShader.cso", buf))
        throw std::runtime_error("Failed load shader file");

    hr = m_d3dDev->CreatePixelShader(
        reinterpret_cast<DWORD*>(buf.data()), 
        &m_PixelShader);
    if (FAILED(hr))
        throw std::runtime_error("Failed CreatePixelShader");
}

void App::LoadTexture()
{
    std::wstring fileName = L"Parrots.png";
    m_Texture = CreateTextureFromFile(m_d3dDev, fileName);
}