/*
* Copyright (C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#undef HAVE_MALLOC_H
#include <SDL.h>
#include <SDL_syswm.h>
#include <stdio.h>
#include <string.h>
#include "OpenglRender/render_api.h"
// #include <EventInjector.h>

static int convert_keysym(int sym); // forward



#ifdef __linux__
#include <X11/Xlib.h>
#endif

char g_szIP[50]; // 端口5559
#ifdef _WIN32
#include <conio.h>
#include <winsock2.h>
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
#else
#include <netinet/in.h>
#include <unistd.h>
int main(int argc, char *argv[])
#endif
{
    int portNum = 22468;
    int winWidth = 320;
    int winHeight = 480;
    int width, height;
    int mouseDown = 0;
    const char* env = getenv("ANDROID_WINDOW_SIZE");
    FBNativeWindowType windowId = NULL;

    int consolePort = 5554;
    // 设置环境变量
    //prependSharedLibraryPath("lib");
#if 1 
#ifdef _WIN32
{
    printf("ip = %s\n", lpCmdLine);
    strcpy(g_szIP, lpCmdLine);
}
#else
    if (argc == 2)
    {
        // 输入地址
        printf("ip = %s\n", argv[1]);
        strcpy(g_szIP, argv[1]);
    }
#endif
#endif
    if (env && sscanf(env, "%dx%d", &width, &height) == 2) {
        winWidth = width;
        winHeight = height;
    }

#ifdef __linux__
    // some OpenGL implementations may call X functions
    // it is safer to synchronize all X calls made by all the
    // rendering threads. (although the calls we do are locked
    // in the FrameBuffer singleton object).
    //XInitThreads();
#endif

    //
    // Inialize SDL window 
    //
    if (SDL_Init(SDL_INIT_NOPARACHUTE | SDL_INIT_VIDEO)) {
        fprintf(stderr,"SDL init failed: %s\n", SDL_GetError());
        return -1;
    } 

    #if 0
    SDL_Surface *surface = SDL_SetVideoMode(winWidth, winHeight, 32, SDL_SWSURFACE);
    if (surface == NULL) {
        fprintf(stderr,"Failed to set video mode: %s\n", SDL_GetError());
        return -1;
    }
    #else
	SDL_WM_SetCaption("My Game Window", "game");  
	SDL_Surface *surface = SDL_SetVideoMode(winWidth, winHeight, 32, /*SDL_FULLSCREEN | SDL_OPENGL*/SDL_SWSURFACE); 
    if (surface == NULL) {
        fprintf(stderr,"Failed to set video mode: %s\n", SDL_GetError());
        return -1;
    }
    #endif

    SDL_SysWMinfo  wminfo;
    memset(&wminfo, 0, sizeof(wminfo));
    SDL_GetWMInfo(&wminfo);
#ifdef _WIN32
    windowId = wminfo.window;
    WSADATA  wsaData;
    int      rc = WSAStartup( MAKEWORD(2,2), &wsaData);
    if (rc != 0) {
            printf( "could not initialize Winsock\n" );
    }
#elif __linux__
    windowId = wminfo.info.x11.window;
#elif __APPLE__
    windowId = wminfo.nsWindowPtr;
#endif

    printf("initializing renderer process\n");
#if 0	//on debug
    //
    // initialize OpenGL renderer to render in our window
    //
    initLibrary();
    char addr[256];
    bool inited = initOpenGLRenderer(winWidth, winHeight, addr, sizeof(addr));
    if (!inited) {
        return -1;
    }
    printf("renderer process started\n");

    float zRot = 0.0f;
    inited = createOpenGLSubwindow(windowId, 0, 0,
                                   winWidth, winHeight, zRot);
    if (!inited) {
        printf("failed to create OpenGL subwindow\n"); 
        stopOpenGLRenderer();
        return -1;
    }
    // new renderer
    #if 1
    {
		static emugl::RenderLibPtr sRenderLib = nullptr;
		// step 1
		sRenderLib = initLibrary();
		if (!sRenderLib) {
		    derror("OpenGLES initialization failed!");
		    goto BAD_EXIT;
		}
		// step 2
		sRenderLib->setRenderer(1/*emuglConfig_get_current_renderer()*/);	// SELECTED_RENDERER_HOST -gpu on
		// sRenderLib->setAvdInfo(guestPhoneApi, guestApiLevel);
		//sRenderLib->setCrashReporter(&crashhandler_die_format);	// 崩溃打印信息并exit(1);
		#if 0
		void crashhandler_die_format(const char* format, ...) {
			char message[2048] = {};
			va_list args;

			va_start(args, format);
			vsnprintf(message, sizeof(message) - 1, format, args);
			va_end(args);

			crashhandler_die(message);
		}
		sRenderLib->setFeatureController(&android::featurecontrol::isEnabled);	// ???
		sRenderLib->setSyncDevice(goldfish_sync_create_timeline,
		        goldfish_sync_create_fence,
		        goldfish_sync_timeline_inc,
		        goldfish_sync_destroy_timeline,
		        goldfish_sync_register_trigger_wait,
		        goldfish_sync_device_exists);

		emugl_logger_struct logfuncs;
		logfuncs.coarse = android_opengl_logger_write;
		logfuncs.fine = android_opengl_cxt_logger_write;
		sRenderLib->setLogger(logfuncs);	// 设置日志
		emugl_dma_ops dma_ops;
		dma_ops.add_buffer = android_goldfish_dma_ops.add_buffer;
		dma_ops.remove_buffer = android_goldfish_dma_ops.remove_buffer;
		dma_ops.get_host_addr = android_goldfish_dma_ops.get_host_addr;
		dma_ops.invalidate_host_mappings = android_goldfish_dma_ops.invalidate_host_mappings;
		dma_ops.unlock = android_goldfish_dma_ops.unlock;
		sRenderLib->setDmaOps(dma_ops);
		#endif

		sRenderer = sRenderLib->initRenderer(width, height, sRendererUsesSubWindow);
		if (!sRenderer) {
		    D("Can't start OpenGLES renderer?");
		    return -1;
		}
		
		char addr[256];
		StartOpenglServer(addr, 256);

		// after initRenderer is a success, the maximum GLES API is calculated depending
		// on feature control and host GPU support. Set the obtained GLES version here.
		//if (glesMajorVersion_out && glesMinorVersion_out)
		//    sRenderLib->getGlesVersion(glesMajorVersion_out, glesMinorVersion_out);
		
		// step 3
		// sRenderer->setPostCallback(onPost, onPostContext); // for mac
		bool success = sRenderer->showOpenGLSubwindow(
		        win, wx, wy, ww, wh, fbw, fbh, dpr, rotation); // 0 0
		
    }
    #endif
#endif //off debug
EXIT:
    //
    // stop the renderer
    //
    printf("stopping the renderer process\n");
    stopOpenGLRenderer();

    return 0;
}

