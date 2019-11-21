#ifndef ABI_BASE_FILE_PATH_H_
#define ABI_BASE_FILE_PATH_H_
//////////////////////////////////////////////////////////////////////////
#include "base/basetypes.h"
//////////////////////////////////////////////////////////////////////////
namespace ABI{
	namespace base{
		class Path
		{
		public:
			Path(void);
			Path(const wchar_t* path,const wchar_t* name,const unsigned long path_type = CSIDL_PROGRAM_FILES);
			~Path(void);
			void SplitFile(const wchar_t* filename,unsigned long count);
			bool BackupFile(const wchar_t* current_name);
			std::wstring GetCurrentPath();
			void set_split_pub_path(const wchar_t* path);
			void set_backup_file_path(const wchar_t* path,const wchar_t* name,const unsigned long path_type = CSIDL_PROGRAM_FILES);
			std::wstring backup_file_path() const;
		private:
			std::wstring GetFileName(const wchar_t* ext,const unsigned int index);
			std::wstring split_pub_path() const;
			std::wstring split_pub_path_;
			std::wstring backup_file_path_;
		};
		class AppendPath
		{
		public:
			AppendPath(const std::wstring& name){
				ABI::base::Path paths;
				directory_name_.resize(0);
				directory_name_.append(paths.GetCurrentPath()+name);
			}
			~AppendPath(){
				directory_name_.resize(0);
			}
			std::wstring directory_name() const{
				return directory_name_;
			}
		private:
			std::wstring directory_name_;
		};
	}
}
//////////////////////////////////////////////////////////////////////////
#endif
