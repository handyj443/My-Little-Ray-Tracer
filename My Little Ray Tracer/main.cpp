#include <Windows.h>
#include <Windowsx.h>

#include <cmath>
#include <vector>
#include <algorithm>
#include "geometry.h"

#define M_PI 3.14159265
using uint = unsigned;
using uint8 = UINT8;
using uint32 = UINT32;

using namespace std;

//______________________________________________________________________________
// GLOBAL DATA STRUCTURES
struct ScreenBuffer
{
	BITMAPINFO info;
	void *pMemory;
	uint width;
	uint height;
	uint bytesPerPixel;
	uint bytesPerRow;
};
struct Color
{
	uint8 r, g, b, a;
};

//______________________________________________________________________________
// GLOBAL VARIABLES
HWND ghMainWnd;
uint gViewWidth = 1280;
uint gViewHeight = 720;
ScreenBuffer gSBuffer;

//______________________________________________________________________________
// FORWARD DECLARATIONS
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void RecreateScreenBuffer(ScreenBuffer *pBuffer, uint width, uint height);
void RenderToWindow(ScreenBuffer *pSBuffer);
void WritePixel(ScreenBuffer *pSBuffer, uint i, uint j, Color color);

//______________________________________________________________________________
// MAIN PROCEDURE
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR pCmdLine, int nShow)
{
	// Create and register window class, and create window
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"My Little Ray Tracer";
	if (!RegisterClass(&wc))
	{
		MessageBox(0, L"RegisterClass FAILED", 0, 0);
		return -1;
	}

	ghMainWnd = CreateWindow(L"My Little Ray Tracer", L"My Little Ray Tracer",
							 WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
							 gViewWidth, gViewHeight, 0, 0, hInstance, 0);
	if (ghMainWnd == 0)
	{
		MessageBox(0, L"CreateWindow FAILED", 0, 0);
		return -1;
	}
	ShowWindow(ghMainWnd, SW_SHOW);

	// Create the screen buffer
	RecreateScreenBuffer(&gSBuffer, gViewWidth, gViewHeight);

	// Enter the message processing loop
	MSG msg = {};
	BOOL bRet = 1;
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			RenderToWindow(&gSBuffer);
			RedrawWindow(ghMainWnd, 0, 0, RDW_INVALIDATE);
		}
	}
	return (int)msg.wParam;
}

//______________________________________________________________________________
// MESSAGE HANDLING
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{

	case WM_MOUSEMOVE:

		return 0;

	case WM_LBUTTONDOWN:

		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_SIZE:
		gViewWidth = LOWORD(lParam);
		gViewHeight = HIWORD(lParam);

		return 0;

	case WM_PAINT:
		PAINTSTRUCT paint;
		HDC dc = BeginPaint(hWnd, &paint);
		StretchDIBits(dc, 0, 0, gViewWidth, gViewHeight, 0, 0, gSBuffer.width,
					  gSBuffer.height, gSBuffer.pMemory, &gSBuffer.info,
					  DIB_RGB_COLORS, SRCCOPY);
		EndPaint(hWnd, &paint);
		return 0;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

//______________________________________________________________________________
// RENDERING
void RecreateScreenBuffer(ScreenBuffer *pBuffer, uint width, uint height)
{
	if (pBuffer->pMemory)
	{
		VirtualFree(pBuffer->pMemory, 0, MEM_RELEASE);
	}
	pBuffer->width = width;
	pBuffer->height = height;
	pBuffer->bytesPerPixel = 4;
	pBuffer->bytesPerRow = pBuffer->width * pBuffer->bytesPerPixel;
	pBuffer->info.bmiHeader.biSize = sizeof(pBuffer->info.bmiHeader);
	pBuffer->info.bmiHeader.biWidth = pBuffer->width;
	pBuffer->info.bmiHeader.biHeight = -(int)pBuffer->height;
	pBuffer->info.bmiHeader.biPlanes = 1;
	pBuffer->info.bmiHeader.biBitCount = 32;
	pBuffer->info.bmiHeader.biCompression = BI_RGB;

	int BitmapMemorySize =
		pBuffer->bytesPerPixel * pBuffer->width * pBuffer->height;
	pBuffer->pMemory = VirtualAlloc(0, BitmapMemorySize,
									MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

void RenderToWindow(ScreenBuffer *pSBuffer)
{
	// Clear buffer
	memset(pSBuffer->pMemory, 0, pSBuffer->height * pSBuffer->bytesPerRow);

	// Render a solid color
	Color blue = { 0, 0, 255, 255 };
	for (uint row = 0; row < pSBuffer->width; ++row)
	{
		for (uint col = 0; col < pSBuffer->height; ++col)
		{
			WritePixel(pSBuffer, row, col, blue);
		}
	}
}

void inline WritePixel(ScreenBuffer *pSBuffer, uint i, uint j, Color color)
{
	if (i < gViewWidth && j < gViewHeight)
	{
		uint32 *pixel =
			(uint32 *)pSBuffer->pMemory + (j * pSBuffer->bytesPerRow >> 2) + i;
		*pixel = (color.r << 16) | (color.g << 8) | color.b;
	}
}