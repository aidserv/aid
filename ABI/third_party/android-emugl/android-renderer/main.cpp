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

//#include "android/utils/dll.h"
#include "OpenglRender/render_api.h"
#include "OpenglRender/render_api_functions.h"
#include "WindowSurface.h"
#include <EventInjector.h>
static int convert_keysym(int sym);
using namespace android;
//#include <SDL.h>
//#include <SDL_syswm.h>
#if 0
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
#if 0
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
#endif

char g_szIP[50]; // 端口5559

#ifdef _WIN32
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
#else
#include <netinet/in.h>
#include <unistd.h>
int main(int argc, char **argv)
#endif
{
#ifdef _WIN32
    WSADATA  wsaData;
    int      rc = WSAStartup( MAKEWORD(2,2), &wsaData);
    if (rc != 0) {
            printf( "could not initialize Winsock\n" );
    }
    printf("init sock success\n");
#endif
	// Create WindowSurface
    WindowSurface wsurface;
    NativeWindowType display_window;
    display_window = wsurface.getSurface();
    
    // wx wy 偏移
    // ww wh 长宽
    #define WW 1920
    #define WH 1080
    int wx=0, wy=0, ww=WW, wh=WH, fbw=WW, fbh=WH; 
    float dpr=1.000000f, rotation=0.000000f;
	// call 
	emugl::RenderLib* sRenderLib = initLibrary();
	
    sRenderLib->setRenderer((SelectedRenderer)5);
    
    bool sRendererUsesSubWindow = true;
	printf("sRendererUsesSubWindow=%d width=%d, height = %d\n", sRendererUsesSubWindow, ww, wh);
    emugl::Renderer* sRenderer = sRenderLib->initRenderer(ww, wh, sRendererUsesSubWindow);
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
        // sRenderLib->getGlesVersion(&glesMajorVersion_out, &glesMinorVersion_out);
   	}
   	// log version
   	emugl::Renderer::HardwareStrings res = sRenderer->getHardwareStrings();
   	printf("\ngetHardwareStrings vendor=%s renderer=%s version=%s\n", 
   	res.vendor.c_str(), 
    res.renderer.c_str(), 
    res.version.c_str());
   	
   	
   	// 启动收包线程
    char addr[256];
	StartOpenglServer(addr, 256);
    
    // init sdl  show window
    // FBNativeWindowType win = MyOpendl();
    printf("wx=%d, wy=%d, ww=%d, wh=%d, fbw=%d, fbh=%d, dpr=%f, rotation=%f\n", wx, wy, ww, wh, fbw, fbh, dpr, rotation);
    bool success = sRenderer->showOpenGLSubwindow(
            (FBNativeWindowType)display_window, wx, wy, ww, wh, fbw, fbh, dpr, rotation);
    
    //#ifdef _WIN32
    if (1)
    {
        #ifdef _WIN32
        #define s_addr S_un.S_addr
        #define socklen_t int
        #else
        #define SOCKET int
        #define closesocket close
        #endif
        SOCKET serSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);   
        if(serSocket == -1/*INVALID_SOCKET*/)  
        {  
            printf("socket error !");  
            return 0;  
        }  
      
        sockaddr_in serAddr;  
        serAddr.sin_family = AF_INET;  
        serAddr.sin_port = htons(8888);
        serAddr.sin_addr.s_addr = INADDR_ANY;
        
        if(bind(serSocket, (sockaddr *)&serAddr, sizeof(serAddr)) == -1/*SOCKET_ERROR*/)  
        {  
            printf("bind error !");  
            closesocket(serSocket);  
            return 0;  
        }  
          
        sockaddr_in remoteAddr;  
        int nAddrLen = sizeof(remoteAddr);   
        while (true)  
        {  
            char recvData[255];
            int ret = recvfrom(serSocket, recvData, 255, 0, (sockaddr *)&remoteAddr, (socklen_t*)&nAddrLen);  
            if (ret > 0)  
            {
                recvData[ret] = 0x00;
                // printf(recvData);
                int opcode = recvData[0];
                char *lpBuff = (char*)(&recvData[1]);
                if (opcode == 1)
                {
                    // get ip
                    strcpy(g_szIP, lpBuff);
                    printf("recv ip=%s\n", g_szIP);
                    break;
                }
                printf("lpBuff=%s\n", lpBuff);
            }
            #if 0
            char * sendData = "recv\n";  
            sendto(serSocket, sendData, strlen(sendData), 0, (sockaddr *)&remoteAddr, nAddrLen);      
            #endif
        }  
        closesocket(serSocket);   
        #undef SOCKET
        #undef closesocket
        #undef s_addr
        #undef socklen_t 
    }
    #if 0
    // 控制器
    EventInjector* injector;
    int consolePort = 5554;
    injector = new EventInjector(consolePort);

    // Just wait until the window is closed
    SDL_Event ev;

    for (;;) {
        injector->wait(1000/15);
        injector->poll();

        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
            case SDL_MOUSEBUTTONDOWN:
                if (!mouseDown) {
                    injector->sendMouseDown(ev.button.x, ev.button.y);
                    mouseDown = 1;
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if (mouseDown) {
                    injector->sendMouseUp(ev.button.x,ev.button.y);
                    mouseDown = 0;
                }
                break;
            case SDL_MOUSEMOTION:
                if (mouseDown)
                    injector->sendMouseMotion(ev.button.x,ev.button.y);
                break;

            case SDL_KEYDOWN:
#ifdef __APPLE__
                /* special code to deal with Command-Q properly */
                if (ev.key.keysym.sym == SDLK_q &&
                    ev.key.keysym.mod & KMOD_META) {
                  goto EXIT;
                }
#endif
                injector->sendKeyDown(convert_keysym(ev.key.keysym.sym));

                if (ev.key.keysym.sym == SDLK_KP_MINUS) {
                    subwinWidth /= 2;
                    subwinHeight /= 2;
                    
                    bool stat = destroyOpenGLSubwindow();
                    printf("destroy subwin returned %d\n", stat);
                    stat = createOpenGLSubwindow(windowId,
                                                (winWidth - subwinWidth) / 2,
                                                (winHeight - subwinHeight) / 2,
                                                subwinWidth, subwinHeight, 
                                                zRot);
                    printf("create subwin returned %d\n", stat);
                }
                else if (ev.key.keysym.sym == SDLK_KP_PLUS) {
                    subwinWidth *= 2;
                    subwinHeight *= 2;

                    bool stat = destroyOpenGLSubwindow();
                    printf("destroy subwin returned %d\n", stat);
                    stat = createOpenGLSubwindow(windowId,
                                                (winWidth - subwinWidth) / 2,
                                                (winHeight - subwinHeight) / 2,
                                                subwinWidth, subwinHeight, 
                                                zRot);
                    printf("create subwin returned %d\n", stat);
                }
                else if (ev.key.keysym.sym == SDLK_KP_MULTIPLY) {
                    zRot += 10.0f;
                    setOpenGLDisplayRotation(zRot);
                }
                else if (ev.key.keysym.sym == SDLK_KP_ENTER) {
                    repaintOpenGLDisplay();
                }
                break;
            case SDL_KEYUP:
                injector->sendKeyUp(convert_keysym(ev.key.keysym.sym));
                break;
            case SDL_QUIT:
                goto EXIT;
            }
        }
    }
EXIT:
    //
    // stop the renderer
    //
    printf("stopping the renderer process\n");
    // stopOpenGLRenderer();
#endif
	//
	#if 1
	//MyOpendl();
    for (;;)
    {
        sleep(1);
    }
    #endif
	
    return 0;
}
#if 0
static int convert_keysym(int sym)
{
#define  EE(x,y)   SDLK_##x, EventInjector::KEY_##y,
    static const int keymap[] = {
        EE(LEFT,LEFT)
        EE(RIGHT,RIGHT)
        EE(DOWN,DOWN)
        EE(UP,UP)
        EE(RETURN,ENTER)
        EE(F1,SOFT1)
        EE(ESCAPE,BACK)
        EE(HOME,HOME)
        -1
    };
    int nn;
    for (nn = 0; keymap[nn] >= 0; nn += 2) {
        if (keymap[nn] == sym)
            return keymap[nn+1];
    }
    return sym;
}
#endif
