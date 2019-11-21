#ifndef ABI_BASE_DIRECTORY_PATH_H_
#define ABI_BASE_DIRECTORY_PATH_H_
//////////////////////////////////////////////////////////////////////////
#include "ABI/base/basetypes.h"
//////////////////////////////////////////////////////////////////////////
namespace base{
	namespace internal{
		class Directory
		{
		public:
			std::wstring GetSystemTempPath();
			std::wstring GetDirectoryTempPath(const std::wstring infile);
			std::wstring GetDirectoryPath(const std::wstring infile);
		};
	}
}
//////////////////////////////////////////////////////////////////////////
#endif