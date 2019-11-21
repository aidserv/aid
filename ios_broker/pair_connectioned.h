#ifndef IOS_BROKER_PAIR_CONNECTIONED_H_
#define IOS_BROKER_PAIR_CONNECTIONED_H_
//////////////////////////////////////////////////////////////////////////
#include "ios_broker/atl_dll_main.h"
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
//////////////////////////////////////////////////////////////////////////
namespace ios_broker{
	namespace internal{
		class PairConnectioned
		{
		public:
			enum PropertyVariantBoolType{kBoolType};
			enum PropertyVariantUintType{kUintType};
			enum PropertyVariantStringType{kStringType};
			PairConnectioned(void);
			PairConnectioned(const char* udid,const char* lable);
			~PairConnectioned(void);
			bool BeginConnectioned(const char* udid,const char* lable);
			void EndConnectioned();
			std::wstring GetName();
			std::wstring GetProperty(const char* name,const char* domain,const PropertyVariantStringType& type);
			uint64 GetProperty(const char* name,const char* domain,const PropertyVariantUintType& type);
			bool GetProperty(const char* name,const char* domain,const PropertyVariantBoolType& type);
			idevice_t device_handle();
			lockdownd_client_t client_handle();
		private:
			idevice_t device_handle_;
			lockdownd_client_t client_handle_;
		};
	}
}
//////////////////////////////////////////////////////////////////////////
#endif
