#ifndef PASSPORT_ITUNES_CLIENT_INTERFACE_H_
#define PASSPORT_ITUNES_CLIENT_INTERFACE_H_
//////////////////////////////////////////////////////////////////////////
#include "smartbot/passport/basictypes.h"
#include <cstdint>
#include "smartbot/passport/itunes_download_info.h"
#include "smartbot/memory/alloc_vtable.h"
#include "ABI/base/windows_hardware.h"
//////////////////////////////////////////////////////////////////////////
namespace passport{
	namespace internal{
		unsigned long GetKbSyncId();
		std::string GetLoginText(const std::string& apple_id,const std::string& password);
		std::string GetKeyValue(const std::string& key,const std::string h_table);
		class GenerateAuthenticateOsGUID
		{
		public:
			explicit GenerateAuthenticateOsGUID(){
				char buffer[MAX_PATH] = {0};
				DWORD buf_len = MAX_PATH;
				GetComputerNameA(buffer,&buf_len);
				machine_name_ = buffer;
				machine_guid_ = ABI::base::GetHardwareCookie();
			}
			std::string machine_name() const{
				//test:2015/3/19
				//return "WIN-4GI25B3ETJE";
				return machine_name_;
			}
			std::string machine_guid() const{
				//test:2015/3/19
				//return "8EFFF7FD.86E7195C.00000000.39CF53B5.2350EAA0.C3A8E888.7FAFF8CE";
				return machine_guid_;
			}
		private:
			std::string machine_name_;
			std::string machine_guid_;
			DISALLOW_EVIL_CONSTRUCTORS(GenerateAuthenticateOsGUID);
		};
		class KbSyncIdParameter
		{
		public:
			static void Initialize();
			static DWORD GetKbsyncIDAddress();
			static const char* AllPCMd5();
			static const char* LocalPCMd5();
		};
	}
	class communicates:public memory::DataInterfaceCaller
	{
	public:
		WIN_DLL_API static communicates* singleton();
		WIN_DLL_API virtual void DICallerAllocate(memory::DataInterfaceAllocate* fmgr);
		WIN_DLL_API virtual void DICallerRelease(memory::DataInterfaceAllocate* fmgr);
		WIN_DLL_API void ResetSapSetup(bool x_act_sig);
		WIN_DLL_API bool GenWebPassportAuthFile(const char* file,const char* os_name = NULL,const char* os_guid = NULL);
		WIN_DLL_API bool SendAuthorizeMachineProtocol();
		WIN_DLL_API bool Authenticate(const char* username,const char* password,const char* os_name=NULL,const char* os_guid=NULL);
		WIN_DLL_API bool SendMessage_pendingSongs();
		WIN_DLL_API bool SendMessageLookupPurchasesAppIdList();
		WIN_DLL_API bool SendMessageLookupPurchasesAppInfo(const char* app_id);
		WIN_DLL_API bool SendMessage_registerSuccess();
		WIN_DLL_API bool SendMessage_purchase_login();
		bool SendMessage_finishMachineProvisioning(const char* message);
		WIN_DLL_API bool SendMessage_buyProduct(const char* product_id,iTunesDownloadInfo* download_info,const int try_count=1,bool expense = false);
		WIN_DLL_API bool SongDownloadDone(const char* product_id,iTunesDownloadInfo* download_info);
	private:
		void Initialize();
		void SapSetupInitialize(bool x_act_sig_flag);
		communicates(void);
		~communicates(void);
		DISALLOW_EVIL_CONSTRUCTORS(communicates);
	};
	class tdk_callback
	{
	public:
		WIN_DLL_API tdk_callback();
		WIN_DLL_API ~tdk_callback();
		WIN_DLL_API void Initialize();
		WIN_DLL_API bool SapSetupInitialize(const int x_aa_sig,const char* sign_cert,char* buffer,size_t length);
		WIN_DLL_API bool CalcXAppleActionSignature(char* buffer,size_t length);
		WIN_DLL_API bool CalcXAppleActionSignature(const char* x_aa_sig,const size_t length);
		WIN_DLL_API bool CalcXAppleActionSignature(const char* x_aa_sig,const size_t x_aa_sig_length,char* buffer,size_t length);
	private:
		DISALLOW_EVIL_CONSTRUCTORS(tdk_callback);
	};
}
//////////////////////////////////////////////////////////////////////////
#endif
