#pragma once
#ifdef MATHLIBRARY_EXPORTS
#define MATHLIBRARY_API __declspec(dllexport)
#else
#define MATHLIBRARY_API __declspec(dllimport)
#endif

//enum ErrorAuthorize {
//	authorize_ok,
//	open_afsync_rq_fail,
//	read_afsync_rq_fail,
//	open_afsync_rq_sig_fail,
//	read_afsync_rq_sig_fail,
//	open_afsync_rs_fail,
//	write_afsync_rs_fail,
//	open_afsync_rs_sig_fail,
//	write_afsync_rs_sig_fail,
//	wait_object_fail,
//	device_info_init,
//	generate_rs_fail,
//	unknown_sync_error,
//	start_session_fail,
//	stop_session_fail,
//	device_init_fail
//};


extern "C" MATHLIBRARY_API int aid(const char* udid);   // ·µ»Øenum ErrorAuthorize



