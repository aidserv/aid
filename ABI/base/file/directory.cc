#include "ABI/base/file/directory.h"

namespace base{
	namespace internal{
		std::wstring Directory::GetSystemTempPath(){
			wchar_t buffer[MAX_PATH] = {0};
			GetTempPathW(MAX_PATH,buffer);
			return (std::wstring(buffer));
		}
		std::wstring Directory::GetDirectoryTempPath(const std::wstring infile){
			wchar_t drive[_MAX_DRIVE] = {0};
			wchar_t dir[_MAX_DIR] = {0};
			wchar_t fname[_MAX_FNAME] = {0};
			wchar_t ext[_MAX_EXT] = {0};
			_wsplitpath(infile.c_str(),drive,dir,fname,ext);
			std::wstring outfile(drive);
			outfile.append(dir);
			outfile.append(L"tmp");
			if(!PathFileExistsW(outfile.c_str())){
				CreateDirectoryW(outfile.c_str(),NULL);
			}
			outfile.append(L"\\");
			outfile.append(fname);
			outfile.append(ext);
			return outfile;
		}
		std::wstring Directory::GetDirectoryPath(const std::wstring infile){
			wchar_t drive[_MAX_DRIVE] = {0};
			wchar_t dir[_MAX_DIR] = {0};
			wchar_t fname[_MAX_FNAME] = {0};
			wchar_t ext[_MAX_EXT] = {0};
			_wsplitpath(infile.c_str(),drive,dir,fname,ext);
			std::wstring outfile(drive);
			outfile.append(dir);
			return outfile;
		}
	}
}