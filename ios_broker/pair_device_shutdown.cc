#include "ios_broker/pair_device_shutdown.h"
#include "ios_broker/pair_connectioned.h"
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#include <libimobiledevice/diagnostics_relay.h>

namespace ios_broker{
	bool PairDeviceShutdown::ShutdownDevice(const char* udid){
		bool status = false;
		lockdownd_service_descriptor_t service = NULL;
		internal::PairConnectioned connection(udid,"ShutdownDevice");
		lockdownd_error_t ret = lockdownd_start_service(connection.client_handle(), "com.apple.mobile.diagnostics_relay", &service);
		if(ret != LOCKDOWN_E_SUCCESS){
			ret = lockdownd_start_service(connection.client_handle(), "com.apple.iosdiagnostics.relay", &service);
		}
		if((ret == LOCKDOWN_E_SUCCESS)&&service&&(service->port>0)){
			diagnostics_relay_client_t diagnostics_client = NULL;
			if(diagnostics_relay_client_new(connection.device_handle(), service, &diagnostics_client) != DIAGNOSTICS_RELAY_E_SUCCESS) {
				status = false;
			}
			else{
				if(diagnostics_relay_shutdown(diagnostics_client, DIAGNOSTICS_RELAY_ACTION_FLAG_WAIT_FOR_DISCONNECT) == DIAGNOSTICS_RELAY_E_SUCCESS) {
					status = true;
				}
				else{
					status = false;
				}
				diagnostics_relay_goodbye(diagnostics_client);
			}
			if(diagnostics_client){
				diagnostics_relay_client_free(diagnostics_client);
			}
		}
		if(service){
			lockdownd_service_descriptor_free(service);
			service = NULL;
		}
		connection.EndConnectioned();
		return status;
	}
	bool PairDeviceShutdown::RestartDevice(const char* udid){
		bool status = false;
		lockdownd_service_descriptor_t service = NULL;
		internal::PairConnectioned connection(udid,"RestartDevice");
		lockdownd_error_t ret = lockdownd_start_service(connection.client_handle(), "com.apple.mobile.diagnostics_relay", &service);
		if(ret != LOCKDOWN_E_SUCCESS){
			ret = lockdownd_start_service(connection.client_handle(), "com.apple.iosdiagnostics.relay", &service);
		}
		if((ret == LOCKDOWN_E_SUCCESS)&&service&&(service->port>0)){
			diagnostics_relay_client_t diagnostics_client = NULL;
			if(diagnostics_relay_client_new(connection.device_handle(), service, &diagnostics_client) != DIAGNOSTICS_RELAY_E_SUCCESS) {
				status = false;
			}
			else{
				if(diagnostics_relay_restart(diagnostics_client, DIAGNOSTICS_RELAY_ACTION_FLAG_WAIT_FOR_DISCONNECT) == DIAGNOSTICS_RELAY_E_SUCCESS) {
					status = true;
				}
				else{
					status = false;
				}
				diagnostics_relay_goodbye(diagnostics_client);
			}
			if(diagnostics_client){
				diagnostics_relay_client_free(diagnostics_client);
			}
		}
		if(service){
			lockdownd_service_descriptor_free(service);
			service = NULL;
		}
		connection.EndConnectioned();
		return status;
	}
	bool PairDeviceShutdown::SleepDevice(const char* udid){
		bool status = false;
		lockdownd_service_descriptor_t service = NULL;
		internal::PairConnectioned connection(udid,"SleepDevice");
		lockdownd_error_t ret = lockdownd_start_service(connection.client_handle(), "com.apple.mobile.diagnostics_relay", &service);
		if(ret != LOCKDOWN_E_SUCCESS){
			ret = lockdownd_start_service(connection.client_handle(), "com.apple.iosdiagnostics.relay", &service);
		}
		if((ret == LOCKDOWN_E_SUCCESS)&&service&&(service->port>0)){
			diagnostics_relay_client_t diagnostics_client = NULL;
			if(diagnostics_relay_client_new(connection.device_handle(), service, &diagnostics_client) != DIAGNOSTICS_RELAY_E_SUCCESS) {
				status = false;
			}
			else{
				if(diagnostics_relay_sleep(diagnostics_client) == DIAGNOSTICS_RELAY_E_SUCCESS){
					status = true;
				}
				else{
					status = false;
				}
				diagnostics_relay_goodbye(diagnostics_client);
			}
			if(diagnostics_client){
				diagnostics_relay_client_free(diagnostics_client);
			}
		}
		if(service){
			lockdownd_service_descriptor_free(service);
			service = NULL;
		}
		connection.EndConnectioned();
		return status;
	}
}