// CG_lab3.cpp : Определяет точку входа для приложения.
//

#include "framework.h"
#include "CG_lab3.h"

#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <windowsx.h>
#include <assert.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>
#include <exception>
#pragma comment(lib, "dxgi")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")
#pragma comment( lib, "dxguid.lib")

#define MAX_LOADSTRING 100
#define M_PI 3.14159265358979323846

#define RELEASE_S(p) if (p != NULL) { p->Release(); p = NULL; }

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND gHWnd = NULL;
UINT m_width = 720;
UINT m_height = 720;

ID3D11Device* m_pDevice = NULL;
ID3D11DeviceContext* m_pDeviceContext = NULL;
IDXGISwapChain* m_pSwapChain = NULL;
ID3D11RenderTargetView* m_pBackBufferRTV = NULL;
ID3D11Buffer* m_pIndexBuffer = NULL;
ID3D11Buffer* m_pVertextBuffer = NULL;
ID3D11InputLayout* m_pInputLayout = NULL;
ID3D11VertexShader* m_pVertexShader = NULL;
ID3D11PixelShader* m_pPixelShader = NULL;
ID3D11Buffer* m_pGeomBuffer = NULL;
ID3D11Buffer* m_pSceneBuffer = NULL;

struct Vertex
{
    float x, y, z;
    COLORREF color;
};

struct GeomBuffer
{
    DirectX::XMMATRIX modelMatrix;
};

struct SceneBuffer
{
    DirectX::XMMATRIX vp;
};

float camRotX = 0.0f;
float camRotY = 0.0f;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

bool InitDirectX(HWND hwnd);
void Render();
bool CreateGeometry();
ID3DBlob* CreateShader(const std::string& path, bool isVertexShader);
void CreateInputLayout(ID3DBlob* pCode);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CGLAB3, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    if (!InitDirectX(gHWnd))
    {
        return FALSE;
    }

    if (!CreateGeometry())
    {
        return FALSE;
    }

    ID3DBlob* pVertexShaderCode = CreateShader("VertexShader.vs", true);
    CreateShader("PixelShader.ps", false);

    CreateInputLayout(pVertexShaderCode);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CGLAB3));

    MSG msg;

    bool exit = false;
    while (!exit)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        if (msg.message == WM_QUIT)
        {
            exit = true;
        }

        m_pDeviceContext->ClearState();

        ID3D11RenderTargetView* views[] = { m_pBackBufferRTV };
        m_pDeviceContext->OMSetRenderTargets(1, views, nullptr);

        static const FLOAT BackColor[4] = { 0.0f, 1.0f, 0.9f, 1.0f };
        m_pDeviceContext->ClearRenderTargetView(m_pBackBufferRTV, BackColor);

        Render();

        HRESULT result = m_pSwapChain->Present(0, 0);
        assert(SUCCEEDED(result));
    }

    RELEASE_S(m_pIndexBuffer);
    RELEASE_S(m_pVertextBuffer);
    RELEASE_S(m_pGeomBuffer);
    RELEASE_S(m_pSceneBuffer);
    RELEASE_S(m_pInputLayout);
    RELEASE_S(m_pVertexShader);
    RELEASE_S(m_pPixelShader);
    RELEASE_S(m_pBackBufferRTV);
    RELEASE_S(m_pSwapChain);
    RELEASE_S(m_pDeviceContext);
    RELEASE_S(m_pDevice);

    gHWnd = NULL;

    return (int)msg.wParam;
}

bool InitDirectX(HWND hWnd)
{
    HRESULT result;

    IDXGIFactory* pFactory = NULL;
    result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory);
    assert(SUCCEEDED(result));

    IDXGIAdapter* pSelectedAdapter = NULL;

    if (SUCCEEDED(result))
    {
        IDXGIAdapter* pAdapter = NULL;
        UINT adapterIdx = 0;

        while (SUCCEEDED(pFactory->EnumAdapters(adapterIdx, &pAdapter)))
        {
            DXGI_ADAPTER_DESC desc;

            pAdapter->GetDesc(&desc);

            if (wcscmp(desc.Description, L"Microsoft Basic Render Driver") != 0)
            {
                pSelectedAdapter = pAdapter;
                break;
            }

            pAdapter->Release();
            adapterIdx++;
        }
    }
    assert(pSelectedAdapter != NULL);

    D3D_FEATURE_LEVEL level;
    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };

    if (SUCCEEDED(result))
    {
        UINT flags = 0;
#ifdef _DEBUG
        flags = D3D11_CREATE_DEVICE_DEBUG;
#endif // _DEBUG

        result = D3D11CreateDevice(pSelectedAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL,
            flags, levels, 1, D3D11_SDK_VERSION, &m_pDevice, &level, &m_pDeviceContext);
        assert(level == D3D_FEATURE_LEVEL_11_0);
        assert(SUCCEEDED(result));

    }

    if (SUCCEEDED(result))
    {
        DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
        swapChainDesc.BufferCount = 2;
        swapChainDesc.BufferDesc.Width = m_width;
        swapChainDesc.BufferDesc.Height = m_height;
        swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
        swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.OutputWindow = hWnd;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Windowed = true;
        swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.Flags = 0;

        result = pFactory->CreateSwapChain(m_pDevice, &swapChainDesc, &m_pSwapChain);
        assert(SUCCEEDED(result));
    }

    if (SUCCEEDED(result))
    {
        ID3D11Texture2D* pBackBuffer = NULL;
        result = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = m_pDevice->CreateRenderTargetView(pBackBuffer, NULL, &m_pBackBufferRTV);
            assert(SUCCEEDED(result));

            RELEASE_S(pBackBuffer);
        }
    }

    RELEASE_S(pSelectedAdapter);
    RELEASE_S(pFactory);

    return SUCCEEDED(result);
}

HRESULT SetResourceName(ID3D11DeviceChild* pResource, const std::string name)
{
    return pResource->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)name.length(), name.c_str());
}

bool CreateGeometry()
{
    D3D11_SUBRESOURCE_DATA data;
    HRESULT result;
    D3D11_BUFFER_DESC desc;

    static const Vertex Vertices[] = {
        { -0.5f, -0.5f, -0.5f, RGB(255, 150, 150) },
        { 0.5f, -0.5f, -0.5f, RGB(150, 255, 150) },
        { 0.5f, 0.5f, -0.5f, RGB(150, 150, 255) },
        {-0.5f, 0.5f, -0.5f, RGB(150, 255, 150)},
        { -0.5f, -0.5f, 0.5f, RGB(255, 150, 150) },
        { 0.5f, -0.5f, 0.5f, RGB(150, 255, 150) },
        { 0.5f, 0.5f, 0.5f, RGB(150, 150, 255) },
        { -0.5f, 0.5f, 0.5f, RGB(150, 255, 150) },
    };

    static const USHORT Indices[] = {
        0, 2, 1, 0, 3, 2,
        2, 5, 1, 2, 6, 5,
        4, 6, 7, 4, 5, 6,
        0, 7, 3, 0, 4, 7,
        3, 7, 6, 3, 6, 2,
        0, 5, 4, 0, 1, 5,
    };

    desc = {};
    desc.ByteWidth = sizeof(Vertices);
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    data.pSysMem = &Vertices;
    data.SysMemPitch = sizeof(Vertices);
    data.SysMemSlicePitch = 0;

    result = m_pDevice->CreateBuffer(&desc, &data, &m_pVertextBuffer);

    if (SUCCEEDED(result))
    {
        result = SetResourceName(m_pVertextBuffer, "VertexBuffer");
    }

    if (SUCCEEDED(result))
    {
        desc = {};
        desc.ByteWidth = sizeof(Indices);
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        data.pSysMem = &Indices;
        data.SysMemPitch = sizeof(Indices);
        data.SysMemSlicePitch = 0;

        result = m_pDevice->CreateBuffer(&desc, &data, &m_pIndexBuffer);
    }

    if (SUCCEEDED(result))
    {
        result = SetResourceName(m_pIndexBuffer, "IndexBuffer");
    }

    if (SUCCEEDED(result))
    {
        desc = { 0 };
        desc.ByteWidth = sizeof(GeomBuffer);
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        result = m_pDevice->CreateBuffer(&desc, NULL, &m_pGeomBuffer);
    }

    if (SUCCEEDED(result))
    {
        result = SetResourceName(m_pGeomBuffer, "GeomBuffer");
    }

    if (SUCCEEDED(result))
    {
        desc = { 0 };
        desc.ByteWidth = sizeof(SceneBuffer);
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;
        result = m_pDevice->CreateBuffer(&desc, NULL, &m_pSceneBuffer);
    }

    if (SUCCEEDED(result))
    {
        result = SetResourceName(m_pSceneBuffer, "SceneBuffer");
    }

    return SUCCEEDED(result);
}

ID3DBlob* CreateShader(const std::string& path, bool isVertexShader)
{
    std::ifstream file(path);
    std::ostringstream ss;
    ss << file.rdbuf();
    const std::string& s = ss.str();
    std::vector<char> data(s.begin(), s.end());

    std::string entryPoint = "";
    std::string platform = "";

    if (isVertexShader)
    {
        entryPoint = "vs";
        platform = "vs_5_0";
    }
    else
    {
        entryPoint = "ps";
        platform = "ps_5_0";
    }

    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif // _DEBUG
    ID3DBlob* pCode = nullptr;
    ID3DBlob* pErrMsg = nullptr;

    HRESULT result = D3DCompile(data.data(), data.size(), path.c_str(), nullptr, nullptr, entryPoint.c_str(),
        platform.c_str(), flags, 0, &pCode, &pErrMsg);

    if (!SUCCEEDED(result) && pErrMsg != nullptr)
    {
        OutputDebugStringA((const char*)pErrMsg->GetBufferPointer());
    }
    assert(SUCCEEDED(result));
    if (pErrMsg != NULL)
    {
        pErrMsg->Release();
        pErrMsg = NULL;
    }

    if (isVertexShader)
    {
        result = m_pDevice->CreateVertexShader(pCode->GetBufferPointer(), pCode->GetBufferSize(), nullptr, &m_pVertexShader);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pVertexShader, path.c_str());
        }
    }
    else
    {
        result = m_pDevice->CreatePixelShader(pCode->GetBufferPointer(), pCode->GetBufferSize(), nullptr, &m_pPixelShader);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pPixelShader, path.c_str());
        }
    }

    return pCode;
}

void CreateInputLayout(ID3DBlob* pCode)
{
    static const D3D11_INPUT_ELEMENT_DESC InputDesc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    HRESULT result = m_pDevice->CreateInputLayout(InputDesc, 2, pCode->GetBufferPointer(), pCode->GetBufferSize(), &m_pInputLayout);
    if (SUCCEEDED(result))
    {
        result = SetResourceName(m_pInputLayout, "InputLayout");
    }
}

void Render()
{
    size_t usec = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    static size_t m_usec = usec;

    double elapsedSec = (usec - m_usec) / 1000000.0;

    double angle = elapsedSec * M_PI * 0.5;
    GeomBuffer geomBuffer;

    DirectX::XMMATRIX m = DirectX::XMMatrixRotationAxis(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f), -(float)angle);
    DirectX::XMMATRIX v = DirectX::XMMatrixInverse(nullptr, DirectX::XMMatrixTranslation(0.0f, 0.3f, -3.0f) *
        DirectX::XMMatrixRotationAxis({ 0,1,0 }, camRotX) * DirectX::XMMatrixRotationAxis({ 1,0,0 }, camRotY));

    float f = 100.0f;
    float n = 0.1f;
    float fov = (float)M_PI / 3;
    float c = 1.0f / tanf(fov / 2);
    float aspectRatio = (float)m_height / m_width;
    DirectX::XMMATRIX p = DirectX::XMMatrixPerspectiveLH(tanf(fov / 2) * 2 * n, tanf(fov / 2) * 2 * n * aspectRatio, n, f);
    geomBuffer.modelMatrix = m;
    m_pDeviceContext->UpdateSubresource(m_pGeomBuffer, 0, nullptr, &geomBuffer, 0, 0);

    D3D11_MAPPED_SUBRESOURCE subresource;
    HRESULT result = m_pDeviceContext->Map(m_pSceneBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);
    if (SUCCEEDED(result))
    {
        SceneBuffer& sceneBuffer = *reinterpret_cast<SceneBuffer*>(subresource.pData);
        sceneBuffer.vp = DirectX::XMMatrixMultiply(v, p);
        m_pDeviceContext->Unmap(m_pSceneBuffer, 0);
    }

    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = (FLOAT)m_width;
    viewport.Height = (FLOAT)m_height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    m_pDeviceContext->RSSetViewports(1, &viewport);

    D3D11_RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = m_width;
    rect.bottom = m_height;
    m_pDeviceContext->RSSetScissorRects(1, &rect);

    m_pDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

    ID3D11Buffer* vertextBuffers[] = { m_pVertextBuffer };

    UINT strides[] = { 16 };
    UINT offsets[] = { 0 };
    m_pDeviceContext->IASetVertexBuffers(0, 1, vertextBuffers, strides, offsets);
    m_pDeviceContext->IASetInputLayout(m_pInputLayout);
    m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pDeviceContext->VSSetShader(m_pVertexShader, nullptr, 0);
    m_pDeviceContext->PSSetShader(m_pPixelShader, nullptr, 0);
    ID3D11Buffer* constBuffers[] = { m_pGeomBuffer, m_pSceneBuffer };
    m_pDeviceContext->VSSetConstantBuffers(0, 2, constBuffers);
    m_pDeviceContext->DrawIndexed(36, 0, 0);
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CGLAB3));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_CGLAB3);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    RECT rc;
    rc.left = 0;
    rc.right = m_width;
    rc.top = 0;
    rc.bottom = m_height;

    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, TRUE);

    MoveWindow(hWnd, 0, 0, rc.right - rc.left, rc.bottom - rc.top, TRUE);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    gHWnd = hWnd;

    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static bool g_mousePress = false;
    static int g_mousePrevX = 0;
    static int g_mousePrevY = 0;

    switch (message)
    {
    case WM_SIZE:
    {
        RECT rc;
        GetClientRect(hWnd, &rc);
        UINT width = rc.right - rc.left;
        UINT height = rc.bottom - rc.top;

        if (width != m_width || height != m_height)
        {
            if (m_pBackBufferRTV != NULL)
            {
                m_pBackBufferRTV->Release();
                m_pBackBufferRTV = NULL;
            }

            HRESULT result = m_pSwapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
            if (SUCCEEDED(result))
            {
                m_width = width;
                m_height = height;

                ID3D11Texture2D* pBackBuffer = NULL;
                HRESULT result = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
                if (SUCCEEDED(result))
                {
                    result = m_pDevice->CreateRenderTargetView(pBackBuffer, NULL, &m_pBackBufferRTV);

                    if (pBackBuffer != NULL)
                    {
                        pBackBuffer->Release();
                        pBackBuffer = NULL;
                    }
                }

                assert(SUCCEEDED(result));
            }
        }

        break;
    }
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        // TODO: Add any drawing code that uses hdc here...
        EndPaint(hWnd, &ps);
    }
    break;

    case WM_LBUTTONDOWN:
        g_mousePress = true;
        g_mousePrevX = GET_X_LPARAM(lParam);
        g_mousePrevY = GET_Y_LPARAM(lParam);
        break;

    case WM_LBUTTONUP:
        g_mousePress = false;
        break;

    case WM_MOUSEMOVE:
        if (g_mousePress)
        {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);

            float dx = x - g_mousePrevX;
            float dy = y - g_mousePrevY;

            camRotX += (float)dx / m_width * 5.0f;

            camRotY += (float)dy / m_height * 5.0f;

            g_mousePrevX = x;
            g_mousePrevY = y;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
