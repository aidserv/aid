#include "Afsync.h"
#include "Logger.h"

namespace aid2 {

	Afsync::Afsync(AMDeviceRef deviceHandle)
	{
		m_deviceHandle = deviceHandle;
		auto kAMDRet = (kAMDError)AMDeviceConnect(m_deviceHandle);
		if (kAMDRet) {
			logger.log("AMDeviceConnect failed.deviceHandle:%p", m_deviceHandle);
			throw "new Afsync error:AMDeviceConnect failed.";
		}
		
		AMDeviceStartSession(m_deviceHandle);
		void* serviceHandle;
		CFStringRef sServiceName = CFStringCreateWithCString(NULL, "com.apple.afc", kCFStringEncodingUTF8);
		if (AMDeviceStartService(m_deviceHandle, sServiceName, &serviceHandle, NULL) != 0) {
			logger.log("AMDeviceStartService failed.deviceHandle:%p", m_deviceHandle);
			throw "new Afsync error:AMDeviceStartService failed.";
		}
		CFRelease(sServiceName);
		if (AFCConnectionOpen(serviceHandle, 0, &m_afc) != 0) {
			logger.log("AFCConnectionOpen failed.deviceHandle:%p", m_deviceHandle);
			throw "new Afsync error:AFCConnectionOpen failed.";
		}
		AMDeviceStopSession(m_deviceHandle);
	}
	Afsync::~Afsync()
	{
		AFCConnectionClose(m_afc);
		AMDeviceDisconnect(m_deviceHandle);
	}
	string Afsync::ReadRq()
	{
		return ReadData(m_filename_afsync_rq);
	}
	string Afsync::ReadRqSig()
	{
		return ReadData(m_filename_afsync_rq_sig);
	}
	bool Afsync::WriteRs(const string afsync_rs)
	{
		return WriteData(afsync_rs, m_filename_afsync_rs);
	}
	bool Afsync::WriteRsSig(const string afsync_rs_sig)
	{
		return WriteData(afsync_rs_sig, m_filename_afsync_rs_sig);
	}
	size_t Afsync::AFCFileSize(string path)
	{
		size_t rqSize = 0;
		afc_dictionary* fileinfo = nullptr;
		if (AFCFileInfoOpen(m_afc, path.c_str(), &fileinfo) != 0) {
			return 0;
		}
		char* pKey, * pValue;
		AFCKeyValueRead(fileinfo, &pKey, &pValue);
		while (pKey || pValue) {
			if (pKey == NULL || pValue == NULL)
				break;
			if (!_stricmp(pKey, "st_size")) {
				rqSize = atoi(pValue);
				break;
			}
			AFCKeyValueRead(fileinfo, &pKey, &pValue);
		}
		AFCKeyValueClose(fileinfo);
		return rqSize;
	}
	string Afsync::ReadData(string filename)
	{
		string result = string();
		size_t  fsize = AFCFileSize(filename);
		if (fsize == 0) {
			return result;
		}
		AFCFileRef file = NULL;
		unsigned int ret = AFCFileRefOpen(m_afc, filename.c_str(), AFC_FILEMODE_READ, &file);
		if (ret != 0) {
			return result;
		}
		result.resize(fsize);
		ret = AFCFileRefRead(m_afc, file, (void*)result.data(), &fsize);
		if (ret != 0) {
			return result;
		}
		AFCFileRefClose(m_afc, file);
		return result;
	}
	bool Afsync::WriteData(const string data, string filename)
	{
		AFCFileRef file = NULL;
		if (AFCFileRefOpen(m_afc, filename.c_str(), AFC_FILEMODE_WRITE, &file) != 0) {
			return false;
		}
		unsigned long ret = AFCFileRefWrite(m_afc, file, (void*)data.data(), data.size());
		if (ret != 0) {
			return false;
		}
		AFCFileRefClose(m_afc, file);
		return true;
	}
}