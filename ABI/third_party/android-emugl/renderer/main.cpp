// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.


#include <algorithm>
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#ifdef _WIN32
#include <conio.h>
#include <winsock2.h>
#include <string.h>
#else
#include <netinet/in.h>
#include <unistd.h>
#endif

#include "android/utils/dll.h"
#include "OpenglRender/render_api.h"
#include "OpenglRender/render_api_functions.h"
//#include <SDL.h>
//#include <SDL_syswm.h>
typedef unsigned int Uint32;
typedef unsigned char Uint8;
#define SDLCALL __cdecl
#define	SDL_INIT_TIMER		0x00000001
#define SDL_INIT_AUDIO		0x00000010
#define SDL_INIT_VIDEO		0x00000020
#define SDL_INIT_CDROM		0x00000100
#define SDL_INIT_JOYSTICK	0x00000200
#define SDL_INIT_NOPARACHUTE	0x00100000	/**< Don't catch fatal signals */
#define SDL_INIT_EVENTTHREAD	0x01000000	/**< Not supported on all OS's */
#define SDL_INIT_EVERYTHING	0x0000FFFF


#define SDL_SWSURFACE	0x00000000	/**< Surface is in system memory */
#define SDL_HWSURFACE	0x00000001	/**< Surface is in video memory */
#define SDL_ASYNCBLIT	0x00000004	/**< Use asynchronous blits if possible */
#if 1
// struct
typedef struct SDL_version {
	Uint8 major;
	Uint8 minor;
	Uint8 patch;
} SDL_version;

typedef struct {
  SDL_version version;
  HWND window;                    /* The Win32 display window */
  HGLRC hglrc;                    /* The OpenGL context, if any */
} SDL_SysWMinfo;

#endif
FBNativeWindowType MyOpendl()
{
    // int wx=28, wy=27, ww=1024, wh=768, fbw=1024, fbh=768, dpr=1.000000f, rotation=0.000000f;
    int ww=1024, wh=768;
    FBNativeWindowType windowId = NULL;
    // 加载库
    // void* ld_lib = dlopen("sdl64_so.so", RTLD_NOW);
    char* error = NULL;
    ADynamicLibrary* ld_lib = adynamicLibrary_open("SDL", &error);	// Load SDL.dll
    if (ld_lib == NULL) {
        printf("Could not load library [SDL.dll]: %s\n", error);
        return NULL;    
    }
    typedef int (SDLCALL *PFNSDL_Init)(Uint32 flags);
    typedef void * SDLCALL (*PFNSDL_SetVideoMode)(int width, int height, int bpp, Uint32 flags);
    typedef int SDLCALL (*PFNSDL_GetWMInfo)(SDL_SysWMinfo *info);
    typedef char * SDLCALL (*PFNSDL_GetError)(void);
    
    PFNSDL_Init SDL_Init = (PFNSDL_Init)adynamicLibrary_findSymbol(ld_lib, "SDL_Init", &error);
    if (SDL_Init == NULL)
	{
        printf("Could not find required symbol [SDL_Init]: %s\n", error);
		return NULL;
	}
    PFNSDL_SetVideoMode SDL_SetVideoMode = (PFNSDL_SetVideoMode)adynamicLibrary_findSymbol(ld_lib, "SDL_SetVideoMode", &error);
    if (SDL_SetVideoMode == NULL)
	{
        printf("Could not find required symbol [SDL_SetVideoMode]: %s\n", error);
		return NULL;
	}
    PFNSDL_GetWMInfo SDL_GetWMInfo = (PFNSDL_GetWMInfo)adynamicLibrary_findSymbol(ld_lib, "SDL_GetWMInfo", &error);
    if (SDL_GetWMInfo == NULL)
	{
        printf("Could not find required symbol [SDL_GetWMInfo]: %s\n", error);
		return NULL;
	}
    PFNSDL_GetError SDL_GetError = (PFNSDL_GetError)adynamicLibrary_findSymbol(ld_lib, "SDL_GetError", &error);
    if (SDL_GetError == NULL)
	{
        printf("Could not find required symbol [SDL_GetError]: %s\n", error);
		return NULL;
	}
    
    // call
    
    //
    // Inialize SDL window
    //
    if (SDL_Init(SDL_INIT_NOPARACHUTE | SDL_INIT_VIDEO)) {
        fprintf(stderr,"SDL init failed: %s\n", SDL_GetError());
        return NULL;
    }

    void *surface = SDL_SetVideoMode(ww, wh, 32, SDL_SWSURFACE);
    if (surface == NULL) {
        fprintf(stderr,"Failed to set video mode: %s\n", SDL_GetError());
        return NULL;
    }
    
    SDL_SysWMinfo  wminfo;
    memset(&wminfo, 0, sizeof(wminfo));
    SDL_GetWMInfo(&wminfo);
#ifdef _WIN32
    windowId = wminfo.window;
#endif
    return windowId;
    
#if 0
	typedef FBNativeWindowType (*PFNGetWindowId)(int winWidth, int winHeight);
	PFNGetWindowId GetWindowId = (PFNGetWindowId)adynamicLibrary_findSymbol(ld_lib, "GetWindowId", &error);
    if (GetWindowId != NULL)
	{
        printf("Could not find required symbol [GetWindowId]: %s\n", error);
		return NULL;
	}
	return GetWindowId(ww, wh);
	//android_initOpenglesEmulation();
	//int glesMajorVersion_out;
	//int glesMinorVersion_out;
                              
	//android_startOpenglesRenderer(ww, wh, 0, 19, &glesMajorVersion_out, &glesMinorVersion_out);
    
    //char addr[256];
	//android_StartOpenglServer(addr, 256);
	//android_showOpenglesWindow((void*)(uintptr_t)wid, wx, wy, ww, wh, fbw, fbh, dpr, rotation);
#endif
}

#ifdef _WIN32
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
#else
int main(int argc, char **argv)
#endif
{
    WSADATA  wsaData;
    int      rc = WSAStartup( MAKEWORD(2,2), &wsaData);
    if (rc != 0) {
            printf( "could not initialize Winsock\n" );
    }
    printf("init sock success\n");
    
    int wx=28, wy=27, ww=1024, wh=768, fbw=1024, fbh=768, dpr=1.000000f, rotation=0.000000f;
    char* error = NULL;
    ADynamicLibrary* ld_lib = adynamicLibrary_open("lib64OpenglRender", &error);
    if (ld_lib == NULL) {
        printf("Could not load library [lib64OpenglRender]: %s\n", error);
        return -1;    
    }
    // initLibrary
	typedef emugl::RenderLibPtr (*PFNinitLibrary)();
	PFNinitLibrary initLibrary = (PFNinitLibrary)adynamicLibrary_findSymbol(ld_lib, "initLibrary", &error);
    if (initLibrary == NULL)
	{
        printf("Could not find required symbol [initLibrary]: %s\n", error);
		return -1;
	}
	// RENDER_APICALL int RENDER_APIENTRY StartOpenglServer(char* addr, size_t addrLen)
	typedef int (*PFNStartOpenglServer)(char* addr, size_t addrLen);
	PFNStartOpenglServer StartOpenglServer = (PFNStartOpenglServer)adynamicLibrary_findSymbol(ld_lib, "StartOpenglServer", &error);
    if (StartOpenglServer == NULL)
	{
        printf("Could not find required symbol [StartOpenglServer]: %s\n", error);
		return -1;
	}
	// call 
	emugl::RenderLibPtr sRenderLib = initLibrary();
	
    sRenderLib->setRenderer((SelectedRenderer)5);
    
    bool sRendererUsesSubWindow = true;
	printf("sRendererUsesSubWindow=%d width=%d, height = %d\n", sRendererUsesSubWindow, ww, wh);
    emugl::RendererPtr sRenderer = sRenderLib->initRenderer(ww, wh, sRendererUsesSubWindow);
    if (!sRenderer) { 
        printf("Can't start OpenGLES renderer?");
        return -1;
    }

    // after initRenderer is a success, the maximum GLES API is calculated depending
    // on feature control and host GPU support. Set the obtained GLES version here.
    int glesMajorVersion_out = 2;
    int glesMinorVersion_out = 0;
    
    if (glesMajorVersion_out && glesMinorVersion_out)
    {
        sRenderLib->getGlesVersion(&glesMajorVersion_out, &glesMinorVersion_out);
   	}
   	// 启动收包线程
    char addr[256];
	StartOpenglServer(addr, 256);
    
    // init sdl  show window
    FBNativeWindowType win = MyOpendl();
    printf("wx=%d, wy=%d, ww=%d, wh=%d, fbw=%d, fbh=%d, dpr=%f, rotation=%f\n", wx, wy, ww, wh, fbw, fbh, dpr, rotation);
    bool success = sRenderer->showOpenGLSubwindow(
            win, wx, wy, ww, wh, fbw, fbh, dpr, rotation);
            
	//
	//MyOpendl();
	while(1);
	
    return 0;
}
