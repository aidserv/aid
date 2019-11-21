#ifndef ABI_BASE_MESSAGE_LOOP_H_
#define ABI_BASE_MESSAGE_LOOP_H_
//////////////////////////////////////////////////////////////////////////
namespace ABI{
	namespace base{
		static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		inline static bool CreateWindowClass(HINSTANCE hInstance){
			WNDCLASSEXW wcex;
			wcex.cbSize = sizeof(WNDCLASSEXW);
			wcex.style			= CS_HREDRAW | CS_VREDRAW;
			wcex.lpfnWndProc	= WndProc;
			wcex.cbClsExtra		= 0;
			wcex.cbWndExtra		= 0;
			wcex.hInstance		= hInstance;
			wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(NULL));
			wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
			wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
			wcex.lpszMenuName	= MAKEINTRESOURCE(NULL);
			wcex.lpszClassName	= L"iAuthedDll";
			wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(NULL));
			return (RegisterClassExW(&wcex)!=0);
		}
		inline static bool CreateWindowInstance(HINSTANCE hInstance){
			HWND hWnd = CreateWindowW(L"iAuthedDll",L"#iAuthedDll32760",WS_OVERLAPPEDWINDOW,10,10,10,10,NULL,NULL,hInstance,NULL);
			if(!hWnd){
				return false;
			}
			ShowWindow(hWnd,SW_HIDE);
			UpdateWindow(hWnd);
			return true;
		}
		inline static DWORD __stdcall MsgDispatchThread(LPVOID param){
			MSG msg = {0};
			while(GetMessage(&msg,NULL,0,0)){
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			return 0;
		}
		inline static bool MessageLoop(HINSTANCE hInstance){
			
			if(!CreateWindowClass(hInstance)||!CreateWindowInstance(hInstance)){
				return false;
			}
			else{
				CloseHandle(CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)MsgDispatchThread,NULL,0,NULL));
				return true;
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////
#endif