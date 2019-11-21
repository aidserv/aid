#ifndef ABI_BASE_LOCK_IMPL_H_
#define ABI_BASE_LOCK_IMPL_H_
//////////////////////////////////////////////////////////////////////////
#include "ABI/base/basetypes.h"
//////////////////////////////////////////////////////////////////////////
namespace ABI{
	namespace base {
		class LockImpl{
		public:
			typedef CRITICAL_SECTION OSLockType;
			LockImpl(){
				::InitializeCriticalSectionAndSpinCount(&os_lock_, INFINITE);
			}
			~LockImpl(){
				::DeleteCriticalSection(&os_lock_);
			}
			static LockImpl* GetInstance(){
				static LockImpl* info;
				if (!info) {
					LockImpl* new_info = new LockImpl();
					if (InterlockedCompareExchangePointer(
						reinterpret_cast<PVOID*>(&info), new_info, NULL)) {
							delete new_info;
					}
				}
				return info;
			}
			bool Try(){
				if (::TryEnterCriticalSection(&os_lock_) != FALSE) {
					return true;
				}
				return false;
			}
			void Lock(){
				::EnterCriticalSection(&os_lock_);
			}
			void Unlock(){
				::LeaveCriticalSection(&os_lock_);
			}
			OSLockType* os_lock() { 
				return &os_lock_; 
			}
		private:
			OSLockType os_lock_;
		};
	}  // namespace base
}
#endif
