#include "ios_cracker/apple_connection_device.h"
#include "ABI/ios_authorize/apple_device_auth.h"

namespace ios_cracker{
	ConnectionDevice::ConnectionDevice(const char* udid){
		ABI::internal::WaitDeviceUDID(udid);
		ABI::internal::ConnectIOSDevice(deviceHandleConnected);
	}
	ConnectionDevice::~ConnectionDevice(){
		ABI::internal::CloseIOSDevice(deviceHandleConnected);
	}
}