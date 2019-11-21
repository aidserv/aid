#include "ios_broker/pair_downloae_done_plist.h"
#include <plist/plist.h>

namespace ios_broker{
	PairDownloaeDonePlist* PairDownloaeDonePlist::GetInterface(bool free_exit){
		static PairDownloaeDonePlist* ref_instance;
		if(!ref_instance){
			PairDownloaeDonePlist* new_instance = new PairDownloaeDonePlist;
			if(InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&ref_instance),new_instance,NULL)){
				delete new_instance;
			}
		}
		if(free_exit){
			delete ref_instance;
			ref_instance = NULL;
		}
		return ref_instance;
	}
	void PairDownloaeDonePlist::DIAllocate(){
		install_plist_ = new char[kMaxPlistLength*2];
		memset(install_plist_,0,kMaxPlistLength*2);
		scinfo_data_ = new char[kMaxPlistLength*2];
		memset(scinfo_data_,0,kMaxPlistLength*2);
	}
	void PairDownloaeDonePlist::DIRelease(){
		if(install_plist_){
			delete[] install_plist_;
			install_plist_ = NULL;
			plist_length_ = 0;
		}
		if(scinfo_data_){
			delete[] scinfo_data_;
			scinfo_data_ = NULL;
		}
	}
	bool PairDownloaeDonePlist::ParseITunesMetaData(const char* src,const size_t length){
		plist_t meta_dict = NULL;
		if(src==NULL||!length){
			return false;
		}
		char* metadata = NULL;
		plist_from_xml(src,length,&meta_dict);
		plist_t bname = plist_dict_get_item(meta_dict,"metadata");
		plist_to_xml(bname,&metadata,&plist_length_);
		memmove(install_plist_,metadata,plist_length_);
		return (plist_length_!=0);
	}
	PairDownloaeDonePlist::PairDownloaeDonePlist(void):install_plist_(NULL),plist_length_(kMaxPlistLength*2),scinfo_data_(NULL){
	}
	PairDownloaeDonePlist::~PairDownloaeDonePlist(void){
		DIRelease();
	}
}