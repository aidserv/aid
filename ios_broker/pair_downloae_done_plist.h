#ifndef IOS_BROKER_PAIR_DOWNLOAE_DONE_PLIST_H_
#define IOS_BROKER_PAIR_DOWNLOAE_DONE_PLIST_H_
//////////////////////////////////////////////////////////////////////////
#include "ios_broker/atl_dll_main.h"
#include "smartbot/memory/alloc_vtable.h"
//////////////////////////////////////////////////////////////////////////
namespace ios_broker{
	const unsigned long kMaxPlistLength = 8096;
	class WIN_DLL_API PairDownloaeDonePlist
	{
	public:
		static PairDownloaeDonePlist* GetInterface(bool free_exit = false);
		virtual void DIAllocate();
		virtual void DIRelease();
		bool ParseITunesMetaData(const char* src,const size_t length);
	private:
		char* install_plist_;
		size_t plist_length_;
		char* scinfo_data_;
		PairDownloaeDonePlist(void);
		~PairDownloaeDonePlist(void);
		DISALLOW_EVIL_CONSTRUCTORS(PairDownloaeDonePlist);
	};
}


#endif