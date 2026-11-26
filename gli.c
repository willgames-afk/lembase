#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <GL/gl.h>
#include "wglext.h"
#include <stdio.h>
#include <stdbool.h>

#define FPS_SAMPLES 120
int tickIndex;
int tickSum;
int ticks[FPS_SAMPLES];

LARGE_INTEGER Freq;
bool maximized;
int width;
int height;
HDC dc;

void getMaximized(HWND win) {
	WINDOWPLACEMENT p = {0};
	p.length = sizeof(WINDOWPLACEMENT);
	if (GetWindowPlacement(win, &p)) {
		maximized = (p.showCmd == SW_SHOWMAXIMIZED);
	}
	maximized = false;
}

double averageTick(int newtick) {
	tickSum -= ticks[tickIndex];
	tickSum += newtick;
	ticks[tickIndex] = newtick;
	if (++tickIndex == FPS_SAMPLES) {
		tickIndex = 0;
	}
	return tickSum/FPS_SAMPLES;

}

RECT getTitlebar(HWND win) {
	SIZE titlebarsize = {0};
	HTHEME t = OpenThemeData(win, L"WINDOW");
	GetThemePartSize(t, NULL, WP_CAPTION, CS_ACTIVE, NULL, TS_TRUE, &titlebarsize);
	CloseThemeData(t);
	RECT r;
	GetClientRect(win,&r);
	r.bottom = r.top + titlebarsize.cy + 2;
	if (maximized) {
		int fy = GetSystemMetrics(SM_CYFRAME);
		r.top -= fy;
		r.bottom -= fy;
	}
	return r;
}

HINSTANCE inst = 0;

void draw() {
	//clear screen
	glClearColor(0.0f,0.0f,0.0f,1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	//set up projection for 2d
	glViewport(0,0,width,height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//this has to be offset by half a pixel for correct rendering
	glOrtho(-0.5,width - 0.5,height - 0.5,-0.5, -1,1);

	glBegin(GL_TRIANGLES);
	glColor3f(1.0f,0.0f,0.0f);
	glVertex2i(100,200);
	glColor3f(0.0f,1.0f,0.0f);
	glVertex2i(0,0);
	glColor3f(0.0f,0.0f,1.0f);
	glVertex2i(200,0);
	glEnd();
	glFlush();
}

LONG WINAPI WindowProc(HWND win, UINT msg, WPARAM wp, LPARAM lp) {
	static PAINTSTRUCT ps;
	int fy;
	int padding;
	switch (msg) {
	case WM_NCCALCSIZE:
		getMaximized(win);
		if (!wp) return DefWindowProc(win, msg, wp, lp);
		//UINT dpi = GetDpiForWindow(win);
		int fx = GetSystemMetrics(SM_CXFRAME);
		fy = GetSystemMetrics(SM_CYFRAME);
		padding = 0;//GetSystemMetrics(SM_CXPADDEDBORDER);

		NCCALCSIZE_PARAMS* params = (NCCALCSIZE_PARAMS*)lp;
		RECT* rect = &(params->rgrc[0]);
		rect->right -= fx + padding;
		rect->left += fx + padding;
		rect->bottom -= fy + padding;

		if (maximized) {
			rect->top += fy + padding;
		}

		return 0;
	case WM_SIZE:
		width = LOWORD(lp);
		height = HIWORD(lp);
		return 0;
	case WM_NCHITTEST:
		LRESULT hit = DefWindowProc(win, msg, wp,lp);
		switch (hit) {
			case HTNOWHERE:
			case HTRIGHT:
			case HTLEFT:
			case HTTOPLEFT:
			case HTTOP:
			case HTTOPRIGHT:
			case HTBOTTOMRIGHT:
			case HTBOTTOMLEFT:
			case HTBOTTOM:
				return hit;
		}

		fy = GetSystemMetrics(SM_CYFRAME);
		padding = 0;

		POINT cursor = {0};
		cursor.x = GET_X_LPARAM(lp);
		cursor.y = GET_Y_LPARAM(lp);
		ScreenToClient(win, &cursor);

		if (!maximized && cursor.y > 0 && cursor.y < fy + padding) return HTTOP;
		if (cursor.y < getTitlebar(win).bottom) return HTCAPTION;

		return HTCLIENT;
	case WM_CREATE:
		SetWindowPos(win, NULL, 0,0,0,0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
		break;
	case WM_PAINT:
		draw();
		SwapBuffers(dc);
		RECT everything = {0,0,width,height};
		ValidateRect(win, &everything);
		return 0;
	case WM_CHAR:
		//key pressed
		return 0;
	case WM_CLOSE:
		//close window
		PostQuitMessage(0);
		return 0;
	case WM_SETCURSOR: 
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		break;
	}
	return DefWindowProc(win, msg, wp, lp);
}

int APIENTRY WinMain(HINSTANCE notinst, HINSTANCE prevInst, LPSTR cmd, int cmdshow) {
	
	if (!inst) {
		inst = GetModuleHandle(NULL);
		WNDCLASSEXW wc = {0};
		wc.cbSize = sizeof(wc);
		wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = (WNDPROC) WindowProc;
		wc.hInstance = inst;
		wc.lpszClassName = L"OpenGL";
		if (!RegisterClassExW(&wc)) return 0;
	}
	HWND win = CreateWindowExW(WS_EX_APPWINDOW, L"OpenGL", L"Lemma", WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX |
		 WS_VISIBLE, CW_USEDEFAULT,CW_USEDEFAULT,256,256, NULL, NULL, inst, NULL);
	if (win == NULL) return 0;
	dc = GetDC(win);
	PIXELFORMATDESCRIPTOR pfd = {0};
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_SUPPORT_COMPOSITION | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.iLayerType = PFD_MAIN_PLANE;
	pfd.cColorBits = 32;
	pfd.cAlphaBits = 8;

	int pf = ChoosePixelFormat(dc, &pfd);
	if (pf == 0) return 0;
	if (SetPixelFormat(dc, pf, &pfd) == FALSE) return 0;
	DescribePixelFormat(dc, pf, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

	HGLRC glc = wglCreateContext(dc); // opengl context
	wglMakeCurrent(dc, glc);
	PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC) wglGetProcAddress("wglSwapIntervalEXT");
	if (wglSwapIntervalEXT)wglSwapIntervalEXT(1);
	glViewport(0,0,1,1);
	
	ShowWindow(win, cmdshow);
	MSG msg;         // message
	while(GetMessage(&msg, win, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	wglMakeCurrent(NULL, NULL);
	ReleaseDC(win, dc);
	wglDeleteContext(glc);
	DestroyWindow(win);
	return msg.wParam;
}