#define NOMINMAX
#include <Windows.h>
#include <Windowsx.h>

#include <cmath>
#include <vector>
#include <limits>
#include <memory>
#include <fstream>
#include <random>
#include <algorithm>
#include "geometry.h"
#include "Object.h"



using uint = unsigned;
using uint8 = UINT8;
using uint32 = UINT32;
using namespace std;


//______________________________________________________________________________
// GLOBAL CONSTANTS
const double M_PI = 3.14159265;
const float kInfinity = numeric_limits<float>::max();

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<> uniformDis(0, 1);

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
struct Options
{
	uint32_t width;
	uint32_t height;
	float fov;
	Matrix44f cameraToWorld;
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
void RenderToWindow(ScreenBuffer *pSBuffer, Options options);
void WritePixel(ScreenBuffer *pSBuffer, uint i, uint j, Color color);
void Render(ScreenBuffer *pSBuffer, const Options &options,
			const std::vector<std::unique_ptr<Object>> &objects);

//______________________________________________________________________________
// UTILITY FUNCTIONS
inline float clamp(const float &lo, const float &hi, const float &v)
{
	return std::max(lo, std::min(hi, v));
}
inline float deg2rad(const float &deg)
{
	return deg * (float)M_PI / 180;
}
inline Vec3f mix(const Vec3f &a, const Vec3f& b, const float &mixValue)
{
	return a * (1 - mixValue) + b * mixValue;
}


//______________________________________________________________________________
// RAYTRACING
// Returns true if the ray intersects an object. The variable tNear is set to the closest intersection distance and hitObject
// is a pointer to the intersected object. The variable tNear is set to infinity and hitObject is set null if no intersection
// was found.
bool trace(const Vec3f &orig, const Vec3f &dir, const std::vector<std::unique_ptr<Object>> &objects, float &tNear, const Object *&hitObject)
{
	tNear = kInfinity;
	hitObject = nullptr;
	std::vector<std::unique_ptr<Object>>::const_iterator iter = objects.begin();
	for (; iter != objects.end(); ++iter) {
		float t = kInfinity;
		if ((*iter)->intersect(orig, dir, t) && t < tNear) {
			hitObject = iter->get();
			tNear = t;
		}
	}

	return (hitObject != nullptr);
}

// Compute the color at the intersection point if any (returns background color otherwise)
Vec3f castRay(
	const Vec3f &orig, const Vec3f &dir,
	const std::vector<std::unique_ptr<Object>> &objects)
{
	Vec3f hitColor = 0;
	const Object *hitObject = nullptr;
	float t;
	if (trace(orig, dir, objects, t, hitObject)) {
		Vec3f Phit = orig + dir * t;
		Vec3f Nhit;
		Vec2f tex;
		hitObject->getSurfaceData(Phit, Nhit, tex);
		// Use the normal and texture coordinates for shading
		float scale = 5;
		float pattern = (float)((fmodf(tex.x * scale, 1.0f) > 0.5f) ^ (fmodf(tex.y * scale, 1.0f) > 0.5f));
		hitColor = max(0.f, Nhit.dotProduct(-dir)) * mix(hitObject->color, hitObject->color * 0.8f, pattern);
	}

	return hitColor;
}

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

	// creating the scene (adding objects and lights)
	std::vector<std::unique_ptr<Object>> objects;

	// generate a scene made of random spheres
	uint32_t numSpheres = 32;
	gen.seed(0);
	for (uint32_t i = 0; i < numSpheres; ++i) {
		Vec3f randPos((float)(0.5 - uniformDis(gen)) * 10, (float)(0.5 - uniformDis(gen)) * 10, (float)(0.5 + uniformDis(gen) * 10));
		float randRadius = (float)(0.5 + uniformDis(gen) * 0.5);
		objects.push_back(std::unique_ptr<Object>(new Sphere(randPos, randRadius)));
	}

	// set up render options
	Options options;
	options.width = gViewWidth;
	options.height = gViewHeight;
	options.fov = 51.52f;
	options.cameraToWorld = Matrix44f(0.945519f, 0, -0.325569f, 0, -0.179534f, 0.834209f, -0.521403f, 0, 0.271593f, 0.551447f, 0.78876f, 0, 4.208271f, 8.374532f, 17.932925f, 1);

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
			RenderToWindow(&gSBuffer, options);
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

void RenderToWindow(ScreenBuffer *pSBuffer, Options options)
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

inline void WritePixel(ScreenBuffer *pSBuffer, uint i, uint j, Color color)
{
	if (i < gViewWidth && j < gViewHeight)
	{
		uint32 *pixel =
			(uint32 *)pSBuffer->pMemory + (j * pSBuffer->bytesPerRow >> 2) + i;
		*pixel = (color.r << 16) | (color.g << 8) | color.b;
	}
}

void Render(ScreenBuffer *pSBuffer, const Options &options,
			const std::vector<std::unique_ptr<Object>> &objects)
{
	Vec3f *framebuffer = new Vec3f[options.width * options.height];
	Vec3f *pix = framebuffer;
	float scale = tan(deg2rad(options.fov * 0.5f));
	float imageAspectRatio = options.width / (float)options.height;
	// [comment]
	// Don't forget to transform the ray origin (which is also the camera origin
	// by transforming the point with coordinates (0,0,0) to world-space using the
	// camera-to-world matrix.
	// [/comment]
	Vec3f orig;
	options.cameraToWorld.multVecMatrix(Vec3f(0), orig);
	for (uint32_t j = 0; j < options.height; ++j) {
		for (uint32_t i = 0; i < options.width; ++i) {
			// [comment]
			// Generate primary ray direction. Compute the x and y position
			// of the ray in screen space. This gives a point on the image plane
			// at z=1. From there, we simply compute the direction by normalized
			// the resulting vec3f variable. This is similar to taking the vector
			// between the point on the image plane and the camera origin, which
			// in camera space is (0,0,0):
			//
			// ray.dir = normalize(Vec3f(x,y,-1) - Vec3f(0));
			// [/comment]
#ifdef MAYA_STYLE
			float x = (2 * (i + 0.5f) / (float)options.width - 1) * scale;
			float y = (1 - 2 * (j + 0.5f) / (float)options.height) * scale * 1 / imageAspectRatio;
#else

			float x = (2 * (i + 0.5f) / (float)options.width - 1) * imageAspectRatio * scale;
			float y = (1 - 2 * (j + 0.5f) / (float)options.height) * scale;
#endif
			// [comment]
			// Don't forget to transform the ray direction using the camera-to-world matrix.
			// [/comment]
			Vec3f dir;
			options.cameraToWorld.multDirMatrix(Vec3f(x, y, -1), dir);
			dir.normalize();
			*(pix++) = castRay(orig, dir, objects);
		}
	}

	// Save result to a PPM image (keep these flags if you compile under Windows)
	std::ofstream ofs("./out.ppm", std::ios::out | std::ios::binary);
	ofs << "P6\n" << options.width << " " << options.height << "\n255\n";
	for (uint32_t i = 0; i < options.height * options.width; ++i) {
		char r = (char)(255 * clamp(0, 1, framebuffer[i].x));
		char g = (char)(255 * clamp(0, 1, framebuffer[i].y));
		char b = (char)(255 * clamp(0, 1, framebuffer[i].z));
		ofs << r << g << b;
	}

	ofs.close();

	delete[] framebuffer;
}