#include "base/file/path.h"

namespace ABI{
	namespace base{
		Path::Path(void){
			set_split_pub_path(L"");
			set_backup_file_path(L"",L"");
		}
		Path::Path(const wchar_t* path,const wchar_t* name,const unsigned long path_type){
			set_split_pub_path(L"");
			set_backup_file_path(path,name,path_type);
		}
		Path::~Path(void){
			set_split_pub_path(L"");
			set_backup_file_path(L"",L"");
		}
		void Path::SplitFile(const wchar_t* filename,unsigned long count){
			std::ifstream infile(filename,std::ifstream::binary);
			infile.seekg(0,std::ifstream::end);
			long double size=static_cast<long double>(infile.tellg());
			infile.seekg(0,std::ifstream::beg);
			unsigned long block_size = static_cast<unsigned long>(size/count);
			if(((unsigned long)size%count)!=0){
				count += 1;
			}
			for(unsigned long i=0;i<count;i++){
				char* buffer = new char[block_size+1];
				memset(buffer,0,block_size+1);
				infile.read(buffer,block_size);
				if(!infile.gcount()){
					break;
				}
				std::ofstream outfile(std::wstring(split_pub_path()+GetFileName(L"bin",i)).c_str(),std::ofstream::binary);
				outfile.write(buffer,infile.gcount());
				outfile.close();
			}
		}
		bool Path::BackupFile(const wchar_t* current_name){
			struct _stat buf = {0};
			if(_wstat(backup_file_path().c_str(),&buf)!=0){
				CopyFileW(std::wstring(GetCurrentPath()+current_name).c_str(),backup_file_path().c_str(),FALSE);
			}
			return true;
		}
		std::wstring Path::GetCurrentPath(){
			wchar_t buffer[MAX_PATH] = {0};
			wchar_t drive[_MAX_DRIVE] = {0};
			wchar_t dir[_MAX_DIR] = {0};
			wchar_t fname[_MAX_FNAME] = {0};
			wchar_t ext[_MAX_EXT] = {0};
			GetModuleFileNameW(NULL,buffer,MAX_PATH);
			_wsplitpath_s(buffer,drive,_MAX_DRIVE,dir,_MAX_DIR,fname,_MAX_FNAME,ext,_MAX_EXT);
			return (std::wstring(std::wstring(drive)+std::wstring(dir)));
		}
		void Path::set_split_pub_path(const wchar_t* path){
			split_pub_path_ = path;
		}
		void Path::set_backup_file_path(const wchar_t* path,const wchar_t* name,const unsigned long path_type){
			if(std::wstring(path).length()>=1){
				wchar_t buffer[MAX_PATH] = {0};
				SHGetSpecialFolderPathW(NULL,buffer,path_type,FALSE);
				std::wstring target_path(buffer);
				GetSystemDirectoryW(buffer,MAX_PATH);
				if(wcsnicmp(buffer,target_path.c_str(),3)!=0){
					target_path[0] = buffer[0];
				}
				target_path.append(L"\\");
				target_path.append(path);
				CreateDirectoryW(target_path.c_str(),NULL);
				target_path.append(L"\\");
				backup_file_path_ = target_path.append(name);
			}
		}
		std::wstring Path::backup_file_path() const{
			return (std::wstring(backup_file_path_));
		}
		std::wstring Path::GetFileName(const wchar_t* ext,const unsigned int index){
			wchar_t buffer[256] = {0};
			swprintf(buffer,L"%d.%s",index,ext);
			return (std::wstring(buffer));
		}
		std::wstring Path::split_pub_path() const{
			return (std::wstring(split_pub_path_));
		}
	}
}