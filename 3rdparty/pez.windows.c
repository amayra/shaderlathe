// Pez was developed by Philip Rideout and released under the MIT License.

#define _WIN32_WINNT 0x0500
#define WINVER 0x0500
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#define GL3W_IMPLEMENTATION
#include "gl3w.h"
#include "pez.h"

LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE ignoreMe0, LPSTR ignoreMe1, INT ignoreMe2)
{
    LPCSTR szName = "Pez App";
    WNDCLASSEXA wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L, GetModuleHandle(0), 0, 0, 0, 0, szName, 0 };
	DWORD dwExStyle = WS_EX_APPWINDOW;// | WS_EX_WINDOWEDGE;
	DWORD dwStyle = WS_VISIBLE | WS_CAPTION | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU;
    RECT rect;
    int windowWidth, windowHeight, windowLeft, windowTop;
    HWND hWnd;
    PIXELFORMATDESCRIPTOR pfd;
    HDC hDC;
    HGLRC hRC;
    int pixelFormat;
    int err;
    DWORD previousTime = GetTickCount();
    MSG msg = {0};

    wc.hCursor = LoadCursor(0, IDC_ARROW);
    RegisterClassExA(&wc);

    SetRect(&rect, 0, 0, PEZ_VIEWPORT_WIDTH, PEZ_VIEWPORT_HEIGHT);
    AdjustWindowRectEx(&rect, dwStyle, FALSE, dwExStyle);
    windowWidth = rect.right - rect.left;
    windowHeight = rect.bottom - rect.top;
    windowLeft = GetSystemMetrics(SM_CXSCREEN) / 2 - windowWidth / 2;
    windowTop = GetSystemMetrics(SM_CYSCREEN) / 2 - windowHeight / 2;
    hWnd = CreateWindowExA(0, szName, szName, dwStyle, windowLeft, windowTop, windowWidth, windowHeight, 0, 0, 0, 0);

    // Create the GL context.
    ZeroMemory(&pfd, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    hDC = GetDC(hWnd);
    pixelFormat = ChoosePixelFormat(hDC, &pfd);

    SetPixelFormat(hDC, pixelFormat, &pfd);
    hRC = wglCreateContext(hDC);
    wglMakeCurrent(hDC, hRC);

	err = gl3w_init();
    if ( err == -1)
    {
        PezFatalError("Failed to init GL!");
    }
    PezDebugString("OpenGL Version: %s\n", glGetString(GL_VERSION));

	typedef HGLRC(APIENTRY * PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC hDC, HGLRC hShareContext, const int *attribList);
	static PFNWGLCREATECONTEXTATTRIBSARBPROC pfnCreateContextAttribsARB = 0;
	GLint attribs[] =
	{
		// Here we ask for OpenGL 4.5
		0x2091, 4,
		0x2092, 3,
		// Uncomment this for forward compatibility mode
		//0x2094, 0x0002,
		// Uncomment this for Compatibility profile
		// 0x9126, 0x9126,
		// We are using Core profile here
		0x9126, 0x00000001,
		0
	};
	pfnCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)(wglGetProcAddress("wglCreateContextAttribsARB"));
    HGLRC newRC = pfnCreateContextAttribsARB(hDC, 0, attribs);
    wglMakeCurrent(0, 0);
    wglDeleteContext(hRC);
    hRC = newRC;
    wglMakeCurrent(hDC, hRC);
    const char* szWindowTitle = PezInitialize(PEZ_VIEWPORT_WIDTH, PEZ_VIEWPORT_HEIGHT);
    SetWindowTextA(hWnd, szWindowTitle);


    // -------------------
    // Start the Game Loop
    // -------------------
	int done = 0;
	while (!done)
    {
		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT) { done = 1; break; }
			DispatchMessage(&msg);
		}
            DWORD currentTime = GetTickCount();
            DWORD deltaTime = currentTime - previousTime;
            previousTime = currentTime;
            PezUpdate(deltaTime);
            PezRender();
            SwapBuffers(hDC);
    }
    UnregisterClassA(szName, wc.hInstance);
	ExitProcess(0);


    return 0;
}

LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int x = LOWORD(lParam);
    int y = HIWORD(lParam);
    switch (msg)
    {
        case WM_LBUTTONUP:
            PezHandleMouse(x, y, PEZ_UP);
            break;

        case WM_LBUTTONDOWN:
            PezHandleMouse(x, y, PEZ_DOWN);
            break;

        case WM_MOUSEMOVE:
            PezHandleMouse(x, y, PEZ_MOVE);
            break;
		case WM_CLOSE:
			PostQuitMessage(0);
			break;
        case WM_KEYDOWN:
        {
            switch (wParam)
            {
                case VK_ESCAPE:
                    PostQuitMessage(0);
                    break;
                case VK_OEM_2: // Question Mark / Forward Slash for US Keyboards
                    break;
            }
            break;
        }
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void PezDebugStringW(const wchar_t* pStr, ...)
{
    wchar_t msg[1024] = {0};

    va_list a;
    va_start(a, pStr);

    _vsnwprintf_s(msg, _countof(msg), _TRUNCATE, pStr, a);
    OutputDebugStringW(msg);
}

void PezDebugString(const char* pStr, ...)
{
    char msg[1024] = {0};

    va_list a;
    va_start(a, pStr);

    _vsnprintf_s(msg, _countof(msg), _TRUNCATE, pStr, a);
    OutputDebugStringA(msg);
}

void PezFatalErrorW(const wchar_t* pStr, ...)
{
    wchar_t msg[1024] = {0};

    va_list a;
    va_start(a, pStr);

    _vsnwprintf_s(msg, _countof(msg), _TRUNCATE, pStr, a);
    OutputDebugStringW(msg);
#ifdef _DEBUG
    __debugbreak();
#endif
    exit(1);
}

void PezFatalError(const char* pStr, ...)
{
    char msg[1024] = {0};

    va_list a;
    va_start(a, pStr);

    _vsnprintf_s(msg, _countof(msg), _TRUNCATE, pStr, a);
    OutputDebugStringA(msg);
#ifdef _DEBUG
    __debugbreak();
#endif
    exit(1);
}
