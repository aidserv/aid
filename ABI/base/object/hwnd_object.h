#ifndef ABI_BASE_OBJECT_HWND_OBJECT_H_
#define ABI_BASE_OBJECT_HWND_OBJECT_H_
//////////////////////////////////////////////////////////////////////////
namespace ABI{
	namespace base{
		class HwndObject
		{
		public:
			HwndObject(){
				hwnd_ = NULL;
			}
			HWND hwnd() const{
				return hwnd_;
			}
			void set_hwnd(HWND notify_hwnd){
				hwnd_ = notify_hwnd;
			}
		private:
			HWND hwnd_;
		};
	}
}
//////////////////////////////////////////////////////////////////////////
#endif