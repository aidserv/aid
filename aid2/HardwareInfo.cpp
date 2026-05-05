#include "HardwareInfo.h"
#include <winsock2.h>
#include <iphlpapi.h>
#include <stdlib.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

namespace aid2 {

    std::string HardwareInfo::NetAdapterSerial()
    {
        ULONG adapterInfoSize = 0;
        if (GetAdaptersAddresses(AF_UNSPEC, 0, NULL, NULL, &adapterInfoSize) == ERROR_BUFFER_OVERFLOW) {
            PIP_ADAPTER_ADDRESSES pAdapterAddresses = (PIP_ADAPTER_ADDRESSES)malloc(adapterInfoSize);
            if (pAdapterAddresses) {
                if (GetAdaptersAddresses(AF_UNSPEC, 0, NULL, pAdapterAddresses, &adapterInfoSize) == NO_ERROR) {
                    PIP_ADAPTER_ADDRESSES pCurrAdapter = pAdapterAddresses;
                    while (pCurrAdapter) {
                        if (pCurrAdapter->IfType == IF_TYPE_ETHERNET_CSMACD ||
                            pCurrAdapter->IfType == IF_TYPE_IEEE80211) {
                            if (pCurrAdapter->PhysicalAddressLength == 6) {
                                char macStr[18];
                                snprintf(macStr, sizeof(macStr), "%02X%02X%02X%02X%02X%02X",
                                    pCurrAdapter->PhysicalAddress[0],
                                    pCurrAdapter->PhysicalAddress[1],
                                    pCurrAdapter->PhysicalAddress[2],
                                    pCurrAdapter->PhysicalAddress[3],
                                    pCurrAdapter->PhysicalAddress[4],
                                    pCurrAdapter->PhysicalAddress[5]);
                                free(pAdapterAddresses);
                                return std::string(macStr);
                            }
                        }
                        pCurrAdapter = pCurrAdapter->Next;
                    }
                }
                free(pAdapterAddresses);
            }
        }
        return "";
    }

    unsigned long HardwareInfo::VolumeSerial()
    {
        unsigned long serialNumber = 0;
        char szVolumeName[MAX_PATH + 1];
        char szFileSystemName[MAX_PATH + 1];
        DWORD dwSerialNumber = 0;
        DWORD dwMaxComponentLength = 0;
        DWORD dwFileSystemFlags = 0;

        if (GetVolumeInformationA("C:\\", szVolumeName, sizeof(szVolumeName),
            &dwSerialNumber, &dwMaxComponentLength, &dwFileSystemFlags,
            szFileSystemName, sizeof(szFileSystemName))) {
            serialNumber = dwSerialNumber;
        }

        return serialNumber;
    }

    std::string HardwareInfo::SystemBiosVersion()
    {
        std::string biosVersion;
        HKEY hKey = NULL;

        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "HARDWARE\\DESCRIPTION\\System",
            0, KEY_READ, &hKey) == ERROR_SUCCESS) {

            char szBiosVersion[256] = { 0 };
            DWORD dwSize = sizeof(szBiosVersion) - 1;

            if (RegQueryValueExA(hKey, "SystemBiosVersion", NULL, NULL,
                (LPBYTE)szBiosVersion, &dwSize) == ERROR_SUCCESS) {
                biosVersion = szBiosVersion;
            }

            RegCloseKey(hKey);
        }

        return biosVersion;
    }

    std::string HardwareInfo::ProcessorName()
    {
        std::string processorName;
        HKEY hKey = NULL;

        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
            0, KEY_READ, &hKey) == ERROR_SUCCESS) {

            char szProcessorName[256] = { 0 };
            DWORD dwSize = sizeof(szProcessorName) - 1;

            if (RegQueryValueExA(hKey, "ProcessorNameString", NULL, NULL,
                (LPBYTE)szProcessorName, &dwSize) == ERROR_SUCCESS) {
                processorName = szProcessorName;
            }

            RegCloseKey(hKey);
        }

        return processorName;
    }

    std::string HardwareInfo::WinComputerName()
    {
        std::string computerName;
        char szComputerName[MAX_COMPUTERNAME_LENGTH + 1] = { 0 };
        DWORD dwSize = sizeof(szComputerName);

        if (GetComputerNameA(szComputerName, &dwSize)) {
            computerName = szComputerName;
        }
        return computerName;
    }

    std::string HardwareInfo::HwProfile()
    {
        std::string hwProfile;
        HW_PROFILE_INFOA hwProfileInfo = {};

        if (GetCurrentHwProfileA(&hwProfileInfo)) {
            hwProfile = hwProfileInfo.szHwProfileGuid;
        }
        return hwProfile;
    }

}
