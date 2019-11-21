#ifndef SMARTBOT_ACCOUNT_THIRD_APPLEID_H_
#define SMARTBOT_ACCOUNT_THIRD_APPLEID_H_
//////////////////////////////////////////////////////////////////////////
#include "smartbot/passport/basictypes.h"
//////////////////////////////////////////////////////////////////////////
namespace smartbot{
	namespace account{
		class ThirdAppleId
		{
		public:
			WIN_DLL_API ThirdAppleId();
			WIN_DLL_API ~ThirdAppleId();
			WIN_DLL_API void iToolsAppleId();
		private:
			void InfoLogger(const wchar_t* filename,const char* msg,size_t length);
			inline std::string GenRandom(const unsigned long len){
				char buffer[MAX_PATH] = {0};
				std::string alphabet("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
				std::string str;
				str.reserve(32);
				srand((unsigned)time(NULL));
				for(unsigned long i=0;i<len;++i){
					buffer[i] = alphabet[rand() % alphabet.length()];
				}
				return buffer;
			}
			inline void set_root_directory(){
				wchar_t buffer[MAX_PATH] = {0};
				wchar_t drive[_MAX_DRIVE] = {0};
				wchar_t dir[_MAX_DIR] = {0};
				wchar_t fname[_MAX_FNAME] = {0};
				wchar_t ext[_MAX_EXT] = {0};
				GetModuleFileName(NULL,buffer,MAX_PATH);
				_wsplitpath(buffer,drive,dir,fname,ext);
				std::wstring outfile(drive);
				root_dir_.append(dir);
			}
			inline std::wstring root_directory() const{
				return root_dir_;
			}
			DISALLOW_EVIL_CONSTRUCTORS(ThirdAppleId);
			std::wstring root_dir_;
		};
	}
}
#endif