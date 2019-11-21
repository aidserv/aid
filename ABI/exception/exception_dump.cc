#include "ABI/exception/exception_dump.h"
#include "ABI/base/file/path.h"
//#pragma comment(lib,"common.lib")
//#pragma comment(lib,"crash_generation_client.lib")
//#pragma comment(lib,"exception_handler.lib")
#include "client/windows/handler/exception_handler.h"

namespace ABI{
	namespace carsh_exception{
		const size_t kMaximumLineLength = 256;
		google_breakpad::ExceptionHandler* dump_handler = NULL;
		static bool ShowDumpResults(const wchar_t *dump_path, const wchar_t *id,  
			void *context, EXCEPTION_POINTERS *exinfo,MDRawAssertionInfo *assertion,bool succeeded) {  
				wchar_t* text = new wchar_t[kMaximumLineLength];
				text[0] = L'\0';
				int result = swprintf_s(text,kMaximumLineLength,L"Dump generation request %s\r\n",succeeded ? L"succeeded" : L"failed");
				if(result != -1){
					MessageBoxW(NULL,text,L"Exception!!!!!!",MB_ICONERROR);
				}
				delete[] text;
				dump_handler->WriteMinidump();
				return succeeded;
		}
		void EnableExceptionHandler(){
			std::wstring directory = ABI::base::AppendPath(L"dumps").directory_name();
			if(!PathFileExistsW(directory.c_str())){
				CreateDirectoryW(directory.c_str(),NULL);
			}
			dump_handler = new google_breakpad::ExceptionHandler(directory,NULL,ShowDumpResults, NULL,google_breakpad::ExceptionHandler::HANDLER_ALL);
		}
		void DisableExceptionHandler(){
			if(dump_handler!=NULL){
				delete dump_handler;
				dump_handler = NULL;
			}
		}
	}
}