#include "ios_cracker/atl_dll_main.h"
#include "ios_broker/pair_device_trust.h"
#pragma comment(lib,"ios_broker.lib")
#pragma comment(lib,"ios_cracker.lib")
#include "PhoneCore/interface/phone_udid.h"
#include "PhoneCore/interface/authorize_id.h"

int main(int argc, char* argv[]){
	const wchar_t* udid = L"ecd2ecc6244384dc71ad44fbeaafa18999e4608";
	USES_CONVERSION;
	sdk::SetUDID(argv[1], strlen(argv[1]));
	if (!sdk::AuthorizeId().ToAuthedorize(sdk::GetUDID())){
		return 1;
	}
	else{
		return 0;
	}
	if (ios_broker::PairDeviceTrust().IsUnlockDevice(sdk::GetUDID())){
		if (!sdk::AuthorizeId().ToAuthedorize(sdk::GetUDID())){
			return 1;
		}
		else{
			return 0;
		}
	}
	return 1;
}