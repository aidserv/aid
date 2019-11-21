#ifndef ABI_BASE_LOCK_MUTEX_H_
#define ABI_BASE_LOCK_MUTEX_H_
//////////////////////////////////////////////////////////////////////////
#include <Windows.h>
#include <cassert>
//////////////////////////////////////////////////////////////////////////
namespace ABI{
	namespace base{
		class mutex
		{
			HANDLE h;
		public:
			mutex():h(CreateMutexW(NULL, FALSE, L"synchronization")){
				assert(h);
			}
			mutex(const wchar_t* mutex_name):h(CreateMutexW(NULL, TRUE, mutex_name)){
				assert(h);
			}
			mutex(const mutex* thread_lock){
				h = thread_lock->h;
			}
			virtual ~mutex(){
				assert(CloseHandle(h));
			}
			void lock(){ 
				assert(WaitForSingleObject(h,INFINITE)==WAIT_OBJECT_0);
			}
			bool try_lock(){
				return (WAIT_OBJECT_0 ==WaitForSingleObject(h,0));
			}
			void unlock(){
				assert(ReleaseMutex(h));
			}
			static mutex* instance(){
				static mutex* info;
				if(!info){
					mutex* new_info = new mutex;
					if(InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&info),new_info,NULL)){
						delete new_info;
					}
				}
				return info;
			}
		private:
			HANDLE handle(){
				return h;
			}
		};
	}
}
//////////////////////////////////////////////////////////////////////////
#endif