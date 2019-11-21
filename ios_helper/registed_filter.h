#ifndef IOS_HELPER_REGISTED_FILTER_H_
#define IOS_HELPER_REGISTED_FILTER_H_
//////////////////////////////////////////////////////////////////////////
#include "ios_helper/atl_dll_main.h"
#include "ABI/base/basetypes.h"
#include "ABI/thirdparty/glog/basictypes.h"
//////////////////////////////////////////////////////////////////////////
namespace ios_helper{
	class WIN_DLL_API RegistedFilter
	{
	public:
		explicit RegistedFilter(HINSTANCE hinstance);
		~RegistedFilter();
		bool RequireProcessToken();
		void FixHardwareRegValue();
		void RegistedHookFilter(HINSTANCE hinstance);
		void UnRegistedHookFilter();
		void RegistedProcessHookFilter();
		static RegistedFilter* RegistedInstance(HINSTANCE hinstance = NULL);
	private:
		DISALLOW_EVIL_CONSTRUCTORS(RegistedFilter);
	};
}
//////////////////////////////////////////////////////////////////////////
#endif