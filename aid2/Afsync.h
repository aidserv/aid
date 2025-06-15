#pragma once
#include<string>
#include "iTunesApi/simpleApi.h"

namespace aid2 {
	using namespace std;
	class Afsync
	{
	public:
		Afsync(AMDeviceRef deviceHandle);
		~Afsync();
		string ReadRq();
		string ReadRqSig();
		bool WriteRs(const string afsync_rs);
		bool WriteRsSig(const string afsync_rs_sig);

	private:
		AFCRef m_afc = nullptr;
		AMDeviceRef m_deviceHandle = nullptr;
		
		const string m_filename_afsync_rq = "/AirFair/sync/afsync.rq";
		const string m_filename_afsync_rq_sig = "/AirFair/sync/afsync.rq.sig";
		const string m_filename_afsync_rs = "/AirFair/sync/afsync.rs";
		const string m_filename_afsync_rs_sig = "/AirFair/sync/afsync.rs.sig";

		size_t AFCFileSize(string path);
		string ReadData(string filename);
		bool WriteData(const string data, string filename);
	};
}

