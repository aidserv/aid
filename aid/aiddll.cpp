#include "ABI/ios_authorize/apple_device_auth.h"
#include "aiddll.h"

int aid(const char* udid) {
	return ABI::internal::AuthorizeDevice(udid);
}
