// Copyright (C) 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/base/system/System.h"

#include "android/base/EintrWrapper.h"
#include "android/base/StringFormat.h"
#include "android/base/StringParse.h"
#include "android/base/files/PathUtils.h"
#include "android/base/files/ScopedFd.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/misc/FileUtils.h"
#include "android/base/misc/StringUtils.h"
#include "android/base/threads/Thread.h"
#include "android/utils/tempfile.h"

#ifdef _WIN32
#include "android/base/files/ScopedRegKey.h"
#include "android/base/system/Win32UnicodeString.h"
#include "android/base/system/Win32Utils.h"
#endif

#ifdef _WIN32
#include <shlobj.h>
#include <windows.h>
#include <psapi.h>
#include <shlobj.h>
#endif

#ifdef __APPLE__
#import <Carbon/Carbon.h>
#include <mach/clock.h>
#include <mach/mach.h>
#include <spawn.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#endif  // __APPLE__

#include <algorithm>
#include <array>
#include <chrono>
#include <memory>
#include <vector>

#ifndef _WIN32
#include <fcntl.h>
#include <dirent.h>
#include <pwd.h>
#include <signal.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#endif
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>

// This variable is a pointer to a zero-terminated array of all environment
// variables in the current process.
// Posix requires this to be declared as extern at the point of use
// NOTE: Apple developer manual states that this variable isn't available for
// the shared libraries, and one has to use the _NSGetEnviron() function instead
#ifdef __APPLE__
#include <crt_externs.h>
#define environ (*_NSGetEnviron())
#else
extern "C" char** environ;
#endif

namespace android {
namespace base {

using std::string;
using std::unique_ptr;
using std::vector;

namespace {

struct TickCountImpl {
private:
    System::WallDuration mStartTimeUs;
#ifdef _WIN32
    long long mFreqPerSec = 0;    // 0 means 'high perf counter isn't available'
#endif

public:
    TickCountImpl() {
#ifdef _WIN32
        LARGE_INTEGER freq;
        if (::QueryPerformanceFrequency(&freq)) {
            mFreqPerSec = freq.QuadPart;
        }
#endif
        mStartTimeUs = getUs();
    }

    System::WallDuration getStartTimeUs() const {
        return mStartTimeUs;
    }

    System::WallDuration getUs() const {
#ifdef _WIN32
    if (!mFreqPerSec) {
        return ::GetTickCount() * 1000;
    }
    LARGE_INTEGER now;
    ::QueryPerformanceCounter(&now);
    return (now.QuadPart * 1000000ull) / mFreqPerSec;
#elif defined __linux__
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000ll + ts.tv_nsec / 1000;
#else // MAC
    clock_serv_t clockServ;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &clockServ);
    clock_get_time(clockServ, &mts);
    mach_port_deallocate(mach_task_self(), clockServ);

    return mts.tv_sec * 1000000ll + mts.tv_nsec / 1000;
#endif
    }
};

// This is, maybe, the only static variable that may not be a LazyInstance:
// it holds the actual timestamp at startup, and has to be initialized as
// soon as possible after the application launch.
static const TickCountImpl kTickCount;

}  // namespace

#ifdef _WIN32
// Check if we're currently running under Wine
static bool isRunningUnderWine() {
    // this is the only good way of detecting Wine: it exports a function
    // 'wine_get_version()' from its ntdll.dll
    // Note: the typedef and casting here are for documentation purposes:
    //  if you need to get the actual Wine version, you just already know the
    //  type, calling convention and arguments.
    using wineGetVersionFunc = const char* __attribute__((cdecl)) ();

    // Make sure we don't call FreeLibrary() for this handle as
    // GetModuleHandle() doesn't increment the reference count
    const HMODULE ntDll = ::GetModuleHandleW(L"ntdll.dll");
    if (!ntDll) {
        // some strange version of Windows, definitely not Wine
        return false;
    }

    if (const auto wineGetVersion = reinterpret_cast<wineGetVersionFunc*>(
                ::GetProcAddress(ntDll, "wine_get_version")) != nullptr) {
        return true;
    }
    return false;
}

static bool extractFullPath(std::string* cmd) {
    if (PathUtils::isAbsolute(*cmd)) {
        return true;
    } else {
        // try searching %PATH% and current directory for the binary
        const Win32UnicodeString name(*cmd);
        const Win32UnicodeString extension(PathUtils::kExeNameSuffix);
        Win32UnicodeString buffer(MAX_PATH);

        DWORD size = ::SearchPathW(nullptr, name.c_str(), extension.c_str(),
                          buffer.size() + 1, buffer.data(), nullptr);
        if (size > buffer.size()) {
            // function may ask for more space
            buffer.resize(size);
            size = ::SearchPathW(nullptr, name.c_str(), extension.c_str(),
                                 buffer.size() + 1, buffer.data(), nullptr);
        }
        if (size == 0) {
            // Couldn't find anything matching the passed name
            return false;
        }
        if (buffer.size() != size) {
            buffer.resize(size);
        }
        *cmd = buffer.toString();
    }
    return true;
}

#endif

namespace {

class HostSystem : public System {
public:
    HostSystem() : mProgramDir(), mHomeDir(), mAppDataDir() {}

    virtual ~HostSystem() {}

    virtual const std::string& getProgramDirectory() const {
        if (mProgramDir.empty()) {
#if defined(__linux__)
            char path[1024];
            memset(path, 0, sizeof(path));  // happy valgrind!
            int len = readlink("/proc/self/exe", path, sizeof(path));
            if (len > 0 && len < (int)sizeof(path)) {
                char* x = ::strrchr(path, '/');
                if (x) {
                    *x = '\0';
                    mProgramDir.assign(path);
                }
            }
#elif defined(__APPLE__)
            ProcessSerialNumber psn;
            GetCurrentProcess(&psn);
            CFDictionaryRef dict =
                    ProcessInformationCopyDictionary(&psn, 0xffffffff);
            CFStringRef value = (CFStringRef)CFDictionaryGetValue(
                    dict, CFSTR("CFBundleExecutable"));
            char s[PATH_MAX];
            CFStringGetCString(value, s, PATH_MAX - 1, kCFStringEncodingUTF8);
            char* x = ::strrchr(s, '/');
            if (x) {
                // skip all slashes - there might be more than one
                while (x > s && x[-1] == '/') {
                    --x;
                }
                *x = '\0';
                mProgramDir.assign(s);
            } else {
                mProgramDir.assign("<unknown-application-dir>");
            }
#elif defined(_WIN32)
            Win32UnicodeString appDir(PATH_MAX);
            int len = GetModuleFileNameW(0, appDir.data(), appDir.size());
            mProgramDir.assign("<unknown-application-dir>");
            if (len > 0) {
                if (len > (int)appDir.size()) {
                    appDir.resize(static_cast<size_t>(len));
                    GetModuleFileNameW(0, appDir.data(), appDir.size());
                }
                std::string dir = appDir.toString();
                char* sep = ::strrchr(&dir[0], '\\');
                if (sep) {
                    *sep = '\0';
                    mProgramDir.assign(dir.c_str());
                }
            }
#else
#error "Unsupported platform!"
#endif
        }
        return mProgramDir;
    }

    virtual std::string getCurrentDirectory() const {
#if defined(_WIN32)
        int currentLen = GetCurrentDirectoryW(0, nullptr);
        if (currentLen < 0) {
            // Could not get size of working directory. Something is really
            // fishy here, return an empty string.
            return std::string();
        }
        wchar_t* currentDir =
                static_cast<wchar_t*>(calloc(currentLen + 1, sizeof(wchar_t)));
        if (!GetCurrentDirectoryW(currentLen + 1, currentDir)) {
            // Again, some unexpected problem. Can't do much here.
            // Make the string empty.
            currentDir[0] = L'0';
        }

        std::string result = Win32UnicodeString::convertToUtf8(currentDir);
        ::free(currentDir);
        return result;
#else   // !_WIN32
        char currentDir[PATH_MAX];
        if (!getcwd(currentDir, sizeof(currentDir))) {
            return std::string();
        }
        return std::string(currentDir);
#endif  // !_WIN32
    }

    virtual const std::string& getLauncherDirectory() const {
        if (mLauncherDir.empty()) {
            std::string launcherDirEnv = envGet("ANDROID_EMULATOR_LAUNCHER_DIR");
            if (!launcherDirEnv.empty()) {
                mLauncherDir = launcherDirEnv;
                return mLauncherDir;
            }
            std::string programDir = getProgramDirectory();

            std::string launcherName("emulator");
#ifdef _WIN32
            launcherName += ".exe";
#endif
            std::vector<StringView> pathList = {programDir, launcherName};
            std::string launcherPath = PathUtils::recompose(pathList);

            if (pathIsFile(launcherPath)) {
                mLauncherDir = programDir;
                return mLauncherDir;
            }

            // we are probably executing a qemu2 binary, which live in
            // <launcher-dir>/qemu/<os>-<arch>/
            // look for the launcher in grandparent directory
            auto programDirVector = PathUtils::decompose(programDir);
            if (programDirVector.size() >= 2) {
                programDirVector.resize(programDirVector.size() - 2);
                std::string grandparentDir = PathUtils::recompose(programDirVector);
                programDirVector.push_back(launcherName);
                std::string launcherPath = PathUtils::recompose(programDirVector);
                if (pathIsFile(launcherPath)) {
                    mLauncherDir = grandparentDir;
                    return mLauncherDir;
                }
            }

            mLauncherDir.assign("<unknown-launcher-dir>");
        }
        return mLauncherDir;
    }

    virtual const std::string& getHomeDirectory() const {
        if (mHomeDir.empty()) {
#if defined(_WIN32)
            // NOTE: SHGetFolderPathW always takes a buffer of MAX_PATH size,
            // so don't use a Win32UnicodeString here to avoid un-necessary
            // dynamic allocation.
            wchar_t path[MAX_PATH] = {0};
            // Query Windows shell for known folder paths.
            // SHGetFolderPath acts as a wrapper to KnownFolders;
            // this is preferred for simplicity and XP compatibility.
            if (SUCCEEDED(
                        SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, 0, path))) {
                mHomeDir = Win32UnicodeString::convertToUtf8(path);
            } else {
                // Fallback to windows-equivalent of HOME env var
                std::string homedrive = envGet("HOMEDRIVE");
                std::string homepath = envGet("HOMEPATH");
                if (!homedrive.empty() && !homepath.empty()) {
                    mHomeDir.assign(homedrive);
                    mHomeDir.append(homepath);
                }
            }
#elif defined(__linux__) || (__APPLE__)
            // Try getting HOME from env first
            const char* home = getenv("HOME");
            if (home != NULL) {
                mHomeDir.assign(home);
            } else {
                // If env HOME appears empty for some reason,
                // try getting HOME by querying system password database
                const struct passwd *pw = getpwuid(getuid());
                if (pw != NULL && pw->pw_dir != NULL) {
                    mHomeDir.assign(pw->pw_dir);
                }
            }
#else
#error "Unsupported platform!"
#endif
        }
        return mHomeDir;
    }

    virtual const std::string& getAppDataDirectory() const {
#if defined(_WIN32)
        if (mAppDataDir.empty()) {
            // NOTE: See comment in getHomeDirectory().
            wchar_t path[MAX_PATH] = {0};
            if (SUCCEEDED(
                        SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, path))) {
                mAppDataDir = Win32UnicodeString::convertToUtf8(path);
            } else {
                const wchar_t* appdata = _wgetenv(L"APPDATA");
                if(appdata != NULL) {
                    mAppDataDir = Win32UnicodeString::convertToUtf8(appdata);
                }
            }
        }
#elif defined (__APPLE__)
        if (mAppDataDir.empty()) {
            // The equivalent of AppData directory in MacOS X is
            // under ~/Library/Preferences. Apple does not offer
            // a C/C++ API to query this location (in ObjC cocoa
            // applications NSSearchPathForDirectoriesInDomains
            // can be used), so we apply the common practice of
            // hard coding it
            mAppDataDir.assign(getHomeDirectory());
            mAppDataDir.append("/Library/Preferences");
        }
#elif defined(__linux__)
        ; // not applicable
#else
#error "Unsupported platform!"
#endif
        return mAppDataDir;
    }


    virtual int getHostBitness() const {
#ifdef __x86_64__
        return 64;
#elif defined(_WIN32)
        // Retrieves the path of the WOW64 system directory, which doesn't
        // exist on 32-bit systems.
        // NB: we don't really need the directory, we just want to see if
        //     Windows has it - so let's not even try to pass a buffer that
        //     is long enough; return value is the required buffer length.
        std::array<wchar_t, 1> directory;
        const unsigned len = GetSystemWow64DirectoryW(
                           directory.data(),
                           static_cast<unsigned>(directory.size()));
        if (len == 0) {
            return 32;
        } else {
            return 64;
        }

#else  // !__x86_64__ && !_WIN32
        // This function returns 64 if host is running 64-bit OS, or 32 otherwise.
        //
        // It uses the same technique in ndk/build/core/ndk-common.sh.
        // Here are comments from there:
        //
        // ## On Linux or Darwin, a 64-bit kernel (*) doesn't mean that the
        // ## user-land is always 32-bit, so use "file" to determine the bitness
        // ## of the shell that invoked us. The -L option is used to de-reference
        // ## symlinks.
        // ##
        // ## Note that on Darwin, a single executable can contain both x86 and
        // ## x86_64 machine code, so just look for x86_64 (darwin) or x86-64
        // ## (Linux) in the output.
        //
        // (*) ie. The following code doesn't always work:
        //     struct utsname u;
        //     int host_runs_64bit_OS = (uname(&u) == 0 &&
        //                              strcmp(u.machine, "x86_64") == 0);
        //
        // Note: system() call on MacOS disables SIGINT signal and fails
        //  to restore it back. As of now we don't have 32-bit Darwin binaries
        //  so this code path won't ever happen, but you've been warned.
        //
        if (system("file -L \"$SHELL\" | grep -q \"x86[_-]64\"") == 0) {
                return 64;
        } else if (system("file -L \"$SHELL\" > /dev/null")) {
            fprintf(stderr, "WARNING: Cannot decide host bitness because "
                    "$SHELL is not properly defined; 32 bits assumed.\n");
        }
        return 32;
#endif // !_WIN32
    }

    virtual OsType getOsType() const override {
#ifdef _WIN32
        return OsType::Windows;
#elif defined(__APPLE__)
        return OsType::Mac;
#elif defined(__linux__)
        return OsType::Linux;
#else
        #error getOsType(): unsupported OS;
#endif
    }

    virtual string getOsName() override {
      static string lastSuccessfulValue;
      if (!lastSuccessfulValue.empty()) {
        return lastSuccessfulValue;
      }
#ifdef _WIN32
        using android::base::ScopedRegKey;
        HKEY hkey = 0;
        LONG result =
                RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                             "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                             0, KEY_READ, &hkey);
        if (result != ERROR_SUCCESS) {
          string errorStr =
              StringFormat("Error: RegGetValueW failed %ld %s", result,
                           Win32Utils::getErrorString(result));
          LOG(VERBOSE) << errorStr;
          return errorStr;
        }
        ScopedRegKey hOsVersionKey(hkey);

        DWORD osNameSize = 0;
        const WCHAR productNameKey[] = L"ProductName";
        result = RegGetValueW(hOsVersionKey.get(), nullptr, productNameKey,
                              RRF_RT_REG_SZ, nullptr, nullptr, &osNameSize);
        if (result != ERROR_SUCCESS && ERROR_MORE_DATA != result) {
          string errorStr =
              StringFormat("Error: RegGetValueW failed %ld %s", result,
                           Win32Utils::getErrorString(result));
          LOG(VERBOSE) << errorStr;
          return errorStr;
        }

        Win32UnicodeString osName;
        osName.resize((osNameSize - 1) / sizeof(wchar_t));
        result = RegGetValueW(hOsVersionKey.get(), nullptr, productNameKey,
                              RRF_RT_REG_SZ, nullptr, osName.data(),
                              &osNameSize);
        if (result != ERROR_SUCCESS) {
          string errorStr =
              StringFormat("Error: RegGetValueW failed %ld %s", result,
                           Win32Utils::getErrorString(result));
          LOG(VERBOSE) << errorStr;
          return errorStr;
        }
        lastSuccessfulValue = osName.toString();
        return lastSuccessfulValue;
#elif defined(__APPLE__) || defined(__linux__)
        using android::base::ScopedFd;
        using android::base::trim;
        const auto versionNumFile = android::base::makeCustomScopedPtr(
                tempfile_create(), tempfile_close);

        if (!versionNumFile) {
          string errorStr =
              "Error: Internal error: could not create a temporary file";
          LOG(VERBOSE) << errorStr;
          return errorStr;
        }

        string tempPath = tempfile_path(versionNumFile.get());

        int exitCode = -1;

#if defined(__APPLE__)
        vector<string> command{"sw_vers"};
#else  // __linux__
        vector<string> command{"lsb_release", "-d"};
#endif
        runCommand(command,
                   RunOptions::WaitForCompletion |
                           RunOptions::TerminateOnTimeout |
                           RunOptions::DumpOutputToFile,
                   1000,  // timeout ms
                   &exitCode, nullptr, tempPath);

        if (exitCode) {
          string errorStr = "Could not get host OS product version.";
          LOG(VERBOSE) << errorStr;
          return errorStr;
        }

        ScopedFd tempfileFd(open(tempPath.c_str(), O_RDONLY));
        if (!tempfileFd.valid()) {
            LOG(VERBOSE) << "Could not open" << tempPath << " : "
                         << strerror(errno);
            return "";
        }

        string contents;
        android::readFileIntoString(tempfileFd.get(), &contents);
        if (contents.empty()) {
          string errorStr = StringFormat(
              "Error: Internal error: could not read temporary file '%s'",
              tempPath);
          LOG(VERBOSE) << errorStr;
          return errorStr;
        }
        string result;
#if defined(__APPLE__)
        char productName[256];
        char productVersion[256];
        memset(productName, 0, 256);
        memset(productVersion, 0, 256);
        android::base::SscanfWithCLocale(
                contents.c_str(),
                "ProductName: %[^\n]\nProductVersion:%[^\n]\n", productName,
                productVersion);

        lastSuccessfulValue = StringFormat("%s %s", trim(string(productName)),
                                           trim(string(productVersion)));
#else   // __linux__
        //"lsb_release -d" output is "Description:      [os-product-version]"
        lastSuccessfulValue = trim(contents.substr(12, contents.size() - 12));
#endif  //!__APPLE__
        return lastSuccessfulValue;
#else
#error getOsName(): unsupported OS;
#endif
    }

    virtual bool isRunningUnderWine() const override {
#ifndef _WIN32
        return false;
#else
        static const bool isUnderWine = android::base::isRunningUnderWine();
        return isUnderWine;
#endif
    }

    System::Pid getCurrentProcessId() const override
    {
#ifdef _WIN32
        return ::GetCurrentProcessId();
#else
        return getpid();
#endif
    }

    MemUsage getMemUsage() override {
        MemUsage res = {};
#ifdef _WIN32
        PROCESS_MEMORY_COUNTERS_EX memCounters = {sizeof(memCounters)};

        if (::GetProcessMemoryInfo(::GetCurrentProcess(),
                    reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memCounters),
                    sizeof(memCounters))) {

            uint64_t pageFileUsageCommit =
                memCounters.PagefileUsage
                ? memCounters.PagefileUsage
                : memCounters.PrivateUsage;

            res.resident = memCounters.WorkingSetSize;
            res.resident_max = memCounters.PeakWorkingSetSize;
            res.virt = pageFileUsageCommit;
            res.virt_max = memCounters.PeakPagefileUsage;
        }

        MEMORYSTATUSEX mem = {sizeof(mem)};
        if (::GlobalMemoryStatusEx(&mem)) {
            res.total_phys_memory = mem.ullTotalPhys;
            res.total_page_file = mem.ullTotalPageFile;
        }
#elif defined (__linux__)
        size_t size = 0;
        FILE *fp = fopen("/proc/self/status", "r");
        char buf[256];
        if (fp != NULL) {
            while (fscanf(fp, "%s:", buf) > 0)  {
                if (strcmp(buf, "VmRSS:") == 0) {
                    fscanf(fp, "%lu", &size);
                    res.resident = size * 1024;
                }
                if (strcmp(buf, "VmHWM:") == 0) {
                    fscanf(fp, "%lu", &size);
                    res.resident_max = size * 1024;
                }
                if (strcmp(buf, "VmSize:") == 0) {
                    fscanf(fp, "%lu", &size);
                    res.virt = size * 1024;
                }
                if (strcmp(buf, "VmPeak:") == 0) {
                    fscanf(fp, "%lu", &size);
                    res.virt_max = size * 1024;
                }
            }
            fclose(fp);
        }

        fp = fopen("/proc/meminfo", "r");
        if (fp != NULL) {
            while (fscanf(fp, "%s:", buf) > 0)  {
                if (strcmp(buf, "MemTotal:") == 0) {
                    fscanf(fp, "%lu", &size);
                    res.total_phys_memory = size * 1024;
                }
                if (strcmp(buf, "SwapTotal:") == 0) {
                    fscanf(fp, "%lu", &size);
                    res.total_page_file = size * 1024;
                }
            }
            fclose(fp);
        }
#elif defined(__APPLE__)
        mach_task_basic_info info = {};
        mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
        task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                reinterpret_cast<task_info_t>(&info), &infoCount);

        int mib[2] = { CTL_HW, HW_MEMSIZE };
        uint64_t total_phys = 0;
        size_t len = sizeof(uint64_t);
        sysctl(mib, 2, &total_phys, &len, nullptr, 0);

        res.resident = info.resident_size;
        res.resident_max = info.resident_size_max;
        res.virt = info.virtual_size;
        res.virt_max = 0; // Max virtual NYI for macOS
        res.total_phys_memory = total_phys; // Max virtual NYI for macOS
        res.total_page_file = 0; // Total page file NYI for macOS
#endif
        return res;
    }

    virtual std::vector<std::string> scanDirEntries(
            StringView dirPath,
            bool fullPath = false) const {
        std::vector<std::string> result = scanDirInternal(dirPath);
        if (fullPath) {
            // Pre-pend |dirPath| to each entry.
            std::string prefix = PathUtils::addTrailingDirSeparator(dirPath);
            for (size_t n = 0; n < result.size(); ++n) {
                std::string path = prefix;
                path.append(result[n]);
                result[n] = path;
            }
        }
        return result;
    }

    virtual std::string envGet(StringView varname) const {
#ifdef _WIN32
        Win32UnicodeString varname_unicode(varname);
        const wchar_t* value = _wgetenv(varname_unicode.c_str());
        if (!value) {
            return std::string();
        } else {
            return Win32UnicodeString::convertToUtf8(value);
        }
#else
        const char* value = getenv(varname.c_str());
        if (!value) {
            value = "";
        }
        return std::string(value);
#endif
    }

    virtual void envSet(StringView varname, StringView varvalue) {
#ifdef _WIN32
        std::string envStr =
            StringFormat("%s=%s", varname, varvalue);
        // Note: this leaks the result of release().
        _wputenv(Win32UnicodeString(envStr).release());
#else
        if (varvalue.empty()) {
            unsetenv(varname.c_str());
        } else {
            setenv(varname.c_str(), varvalue.c_str(), 1);
        }
#endif
    }

    virtual bool envTest(StringView varname) const {
#ifdef _WIN32
        Win32UnicodeString varname_unicode(varname);
        const wchar_t* value = _wgetenv(varname_unicode.c_str());
        return value && value[0] != L'\0';
#else
        const char* value = getenv(varname.c_str());
        return value && value[0] != '\0';
#endif
    }

    virtual std::vector<std::string> envGetAll() const override {
        std::vector<std::string> res;
        for (auto env = environ; env && *env; ++env) {
            res.push_back(*env);
        }
        return res;
    }

    virtual bool isRemoteSession(std::string* sessionType) const {
        if (envTest("NX_TEMP")) {
            if (sessionType) {
                *sessionType = "NX";
            }
            return true;
        }
        if (envTest("CHROME_REMOTE_DESKTOP_SESSION")) {
            if (sessionType) {
                *sessionType = "Chrome Remote Desktop";
            }
            return true;
        }
#ifdef _WIN32
        if (GetSystemMetrics(SM_REMOTESESSION)) {
            if (sessionType) {
                *sessionType = "Windows Remote Desktop";
            }
            return true;
        }
#endif  // _WIN32
        return false;
    }

    virtual bool pathExists(StringView path) const {
        return pathExistsInternal(path);
    }

    virtual bool pathIsFile(StringView path) const {
        return pathIsFileInternal(path);
    }

    virtual bool pathIsDir(StringView path) const {
        return pathIsDirInternal(path);
    }

    virtual bool pathCanRead(StringView path) const override {
        return pathCanReadInternal(path);
    }

    virtual bool pathCanWrite(StringView path) const override {
        return pathCanWriteInternal(path);
    }

    virtual bool pathCanExec(StringView path) const override {
        return pathCanExecInternal(path);
    }

    virtual bool deleteFile(StringView path) const override {
        return deleteFileInternal(path);
    }

    virtual bool pathFileSize(StringView path,
            FileSize* outFileSize) const override {
        return pathFileSizeInternal(path, outFileSize);
    }

    virtual Optional<Duration> pathCreationTime(StringView path) const override {
        return pathCreationTimeInternal(path);
    }

    Times getProcessTimes() const override {
        Times res = {};

#ifdef _WIN32
        FILETIME creationTime = {};
        FILETIME exitTime = {};
        FILETIME kernelTime = {};
        FILETIME userTime = {};
        ::GetProcessTimes(::GetCurrentProcess(), &creationTime, &exitTime,
                &kernelTime, &userTime);

        // convert 100-ns intervals from a struct to int64_t milliseconds
        ULARGE_INTEGER kernelInt64;
        kernelInt64.LowPart = kernelTime.dwLowDateTime;
        kernelInt64.HighPart = kernelTime.dwHighDateTime;
        res.systemMs = static_cast<Duration>(kernelInt64.QuadPart / 10000);

        ULARGE_INTEGER userInt64;
        userInt64.LowPart = userTime.dwLowDateTime;
        userInt64.HighPart = userTime.dwHighDateTime;
        res.userMs = static_cast<Duration>(userInt64.QuadPart / 10000);
#else
        tms times = {};
        ::times(&times);
        // convert to milliseconds
        const long int ticksPerSec = ::sysconf(_SC_CLK_TCK);
        res.systemMs = (times.tms_stime * 1000ll) / ticksPerSec;
        res.userMs = (times.tms_utime * 1000ll) / ticksPerSec;
#endif
        res.wallClockMs =
            (kTickCount.getUs() - kTickCount.getStartTimeUs()) / 1000;

        return res;
    }

    time_t getUnixTime() const override {
        return time(NULL);
    }

    Duration getUnixTimeUs() const override {
        timeval tv;
        gettimeofday(&tv, nullptr);
        return tv.tv_sec * 1000000LL + tv.tv_usec;
    }

    WallDuration getHighResTimeUs() const override {
        return kTickCount.getUs();
    }

    void sleepMs(unsigned n) const override {
        Thread::sleepMs(n);
    }

    void yield() const override {
        Thread::yield();
    }

    bool runCommand(const std::vector<std::string>& commandLine,
            RunOptions options,
            System::Duration timeoutMs,
            System::ProcessExitCode* outExitCode,
            System::Pid* outChildPid,
            const std::string& outputFile) override {
        // Sanity check.
        if (commandLine.empty()) {
            return false;
        }

#ifdef _WIN32
        std::vector<std::string> commandLineCopy = commandLine;
        STARTUPINFOW startup = {};
        startup.cb = sizeof(startup);

        if (!extractFullPath(&commandLineCopy[0])) {
            return false;
        }

        if ((options & RunOptions::ShowOutput) == 0 ||
                ((options & RunOptions::DumpOutputToFile) != 0 &&
                 !outputFile.empty())) {
            startup.dwFlags = STARTF_USESHOWWINDOW;

            // 'Normal' way of hiding console output is passing null std handles
            // to the CreateProcess() function and CREATE_NO_WINDOW as a flag.
            // Sadly, in this case Cygwin runtime goes completely mad - its
            // whole FILE* machinery just stops working. E.g., resize2fs always
            // creates corrupted images if you try doing it in a 'normal' way.
            // So, instead, we do the following: run the command in a cmd.exe
            // with stdout and stderr redirected to either nul (for no output)
            // or the specified file (for file output).

            // 1. Find the commmand-line interpreter - which hides behind the
            // %COMSPEC% environment variable
            std::string comspec = envGet("COMSPEC");
            if (comspec.empty()) {
                comspec = "cmd.exe";
            }

            if (!extractFullPath(&comspec)) {
                return false;
            }

            // 2. Now turn the command into the proper cmd command:
            //   cmd.exe /C "command" "arguments" ... >nul 2>&1
            // This executes a command with arguments passed and redirects
            // stdout to nul, stderr is attached to stdout (so it also
            // goes to nul)
            commandLineCopy.insert(commandLineCopy.begin(), "/C");
            commandLineCopy.insert(commandLineCopy.begin(), comspec);

            if ((options & RunOptions::DumpOutputToFile) != RunOptions::Empty) {
                commandLineCopy.push_back(">");
                commandLineCopy.push_back(outputFile);
                commandLineCopy.push_back("2>&1");
            } else if ((options & RunOptions::ShowOutput) == RunOptions::Empty) {
                commandLineCopy.push_back(">nul");
                commandLineCopy.push_back("2>&1");
            }
        }

        PROCESS_INFORMATION pinfo = {};

        std::string args = commandLineCopy[0];
        for (size_t i = 1; i < commandLineCopy.size(); ++i) {
            args += ' ';
            args += android::base::Win32Utils::quoteCommandLine(commandLineCopy[i]);
        }

        Win32UnicodeString commandUnicode(commandLineCopy[0]);
        Win32UnicodeString argsUnicode(args);

        if (!::CreateProcessW(commandUnicode.c_str(), // program path
                    argsUnicode.data(), // command line args, has to be writable
                    nullptr,            // process handle is not inheritable
                    nullptr,            // thread handle is not inheritable
                    FALSE,              // no, don't inherit any handles
                    0,                  // default creation flags
                    nullptr,            // use parent's environment block
                    nullptr,            // use parent's starting directory
                    &startup,           // startup info, i.e. std handles
                    &pinfo)) {
            return false;
        }

        CloseHandle(pinfo.hThread);
        // make sure we close the process handle on exit
        const android::base::Win32Utils::ScopedHandle process(pinfo.hProcess);

        if (outChildPid) {
            *outChildPid = pinfo.dwProcessId;
        }

        if ((options & RunOptions::WaitForCompletion) == 0) {
            return true;
        }

        // We were requested to wait for the process to complete.
        DWORD ret = ::WaitForSingleObject(pinfo.hProcess,
                timeoutMs ? timeoutMs : INFINITE);
        if (ret == WAIT_FAILED || ret == WAIT_TIMEOUT) {
            if ((options & RunOptions::TerminateOnTimeout) != 0) {
                ::TerminateProcess(pinfo.hProcess, 1);
            }
            return false;
        }

        DWORD exitCode;
        auto exitCodeSuccess = ::GetExitCodeProcess(pinfo.hProcess, &exitCode);
        assert(exitCodeSuccess);
        (void)exitCodeSuccess;
        if (outExitCode) {
            *outExitCode = exitCode;
        }
        return true;

#else   // !_WIN32
        sigset_t oldset;
        sigset_t set;
        if (sigemptyset(&set) || sigaddset(&set, SIGCHLD) ||
                pthread_sigmask(SIG_UNBLOCK, &set, &oldset)) {
            return false;
        }
        auto result = runCommandPosix(commandLine, options, timeoutMs,
                outExitCode, outChildPid, outputFile);
        pthread_sigmask(SIG_SETMASK, &oldset, nullptr);
        return result;
#endif  // !_WIN32
    }

    virtual std::string getTempDir() const {
#ifdef _WIN32
        Win32UnicodeString path(PATH_MAX);
        DWORD retval = GetTempPathW(path.size(), path.data());
        if (retval > path.size()) {
            path.resize(static_cast<size_t>(retval));
            retval = GetTempPathW(path.size(), path.data());
        }
        if (!retval) {
            // Best effort!
            return std::string("C:\\Temp");
        }
        path.resize(retval);
        // The result of GetTempPath() is already user-dependent
        // so don't append the username or userid to the result.
        path.append(L"\\AndroidEmulator");
        ::_wmkdir(path.c_str());
        return path.toString();
#else   // !_WIN32
        std::string result;
        const char* tmppath = getenv("ANDROID_TMP");
        if (!tmppath) {
            const char* user = getenv("USER");
            if (user == NULL || user[0] == '\0') {
                user = "unknown";
            }
            result = "/tmp/android-";
            result += user;
        } else {
            result = tmppath;
        }
        ::mkdir(result.c_str(), 0744);
        return result;
#endif  // !_WIN32
    }

#ifndef _WIN32
    bool runCommandPosix(const std::vector<std::string>& commandLine,
            RunOptions options,
            System::Duration timeoutMs,
            System::ProcessExitCode* outExitCode,
            System::Pid* outChildPid,
            const std::string& outputFile) {
        vector<char*> params;
        for (const auto& item : commandLine) {
            params.push_back(const_cast<char*>(item.c_str()));
        }
        params.push_back(nullptr);

        std::string cmd = "";
        if(LOG_IS_ON(VERBOSE)) {
            cmd = "|";
            for (const auto& param : commandLine) {
                cmd += param;
                cmd += " ";
            }
            cmd += "|";
        }

#if defined(__APPLE__)
        int pid = runViaPosixSpawn(commandLine[0].c_str(), params, options,
                outputFile);
#else
        int pid = runViaForkAndExec(commandLine[0].c_str(), params, options,
                outputFile);
#endif  // !defined(__APPLE__)

        if (pid < 0) {
            LOG(VERBOSE) << "Failed to fork for command " << cmd;
            return false;
        }

        if (outChildPid) {
            *outChildPid = pid;
        }

        if ((options & RunOptions::WaitForCompletion) == 0) {
            return true;
        }

        // We were requested to wait for the process to complete.
        int exitCode;
        // Do not use SIGCHLD here because we're not sure if we're
        // running on the main thread and/or what our sigmask is.
        if (timeoutMs == kInfinite) {
            // Let's just wait forever and hope that the child process
            // exits.
            HANDLE_EINTR(waitpid(pid, &exitCode, 0));
            if (outExitCode) {
                *outExitCode = WEXITSTATUS(exitCode);
            }
            return WIFEXITED(exitCode);
        }

        auto startTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::milliseconds::zero();
        while (elapsed.count() < timeoutMs) {
            pid_t waitPid = HANDLE_EINTR(waitpid(pid, &exitCode, WNOHANG));
            if (waitPid < 0) {
                auto local_errno = errno;
                LOG(VERBOSE) << "Error running command " << cmd
                    << ". waitpid failed with |"
                    << strerror(local_errno) << "|";
                return false;
            }

            if (waitPid > 0) {
                if (outExitCode) {
                    *outExitCode = WEXITSTATUS(exitCode);
                }
                return WIFEXITED(exitCode);
            }

            sleepMs(10);
            elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - startTime);
        }

        // Timeout occured.
        if ((options & RunOptions::TerminateOnTimeout) != 0) {
            kill(pid, SIGKILL);
            waitpid(pid, nullptr, WNOHANG);
        }
        LOG(VERBOSE) << "Timed out with running command " << cmd;
        return false;
    }

    int runViaForkAndExec(const char* command,
            const vector<char*>& params,
            RunOptions options,
            const string& outputFile) {
        // If an output file was requested, open it before forking, since
        // creating a file in the child of a multi-threaded process is sketchy.
        //
        // It will be immediately closed in the parent process, and dup2'd into
        // stdout and stderr in the child process.
        int outputFd = 0;
        if ((options & RunOptions::DumpOutputToFile) != RunOptions::Empty) {
            if (outputFile.empty()) {
                LOG(VERBOSE) << "Can not redirect output to empty file!";
                return -1;
            }

            // Ensure the umask doesn't get in the way while creating the
            // output file.
            mode_t old = umask(0);
            outputFd = open(outputFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC,
                    0700);
            umask(old);
            if (outputFd < 0) {
                LOG(VERBOSE) << "Failed to open file to redirect stdout/stderr";
                return -1;
            }
        }

        int pid = fork();

        if (pid != 0) {
            if (outputFd > 0) {
                close(outputFd);
            }
            // Return the child's pid / error code to parent process.
            return pid;
        }

        // In the child process.
        // Do not do __anything__ except execve. That includes printing to
        // stdout/stderr. None of it is safe in the child process forked from a
        // parent with multiple threads.
        if ((options & RunOptions::DumpOutputToFile) != RunOptions::Empty) {
            dup2(outputFd, 1);
            dup2(outputFd, 2);
            close(outputFd);
        } else if ((options & RunOptions::ShowOutput) == RunOptions::Empty) {
            // We were requested to RunOptions::HideAllOutput
            int fd = open("/dev/null", O_WRONLY);
            if (fd > 0) {
                dup2(fd, 1);
                dup2(fd, 2);
                close(fd);
            }
        }

        // We never want to forward our stdin to the child process. On the other
        // hand, closing it can confuse some programs.
        int fd = open("/dev/null", O_RDONLY);
        if (fd > 0) {
            dup2(fd, 0);
            close(fd);
        }

        if (execvp(command, params.data()) == -1) {
            // emulator doesn't really like exit calls from a forked process
            // (it just hangs), so let's just kill it
            if (raise(SIGKILL) != 0) {
                exit(RunFailed);
            }
        }
        // Should not happen, but let's keep the compiler happy
        return -1;
    }

#if defined(__APPLE__)
    int runViaPosixSpawn(const char* command,
            const vector<char*>& params,
            RunOptions options,
            const string& outputFile) {
        posix_spawnattr_t attr;
        if (posix_spawnattr_init(&attr)) {
            LOG(VERBOSE) << "Failed to initialize spawnattr obj.";
            return -1;
        }
        // Automatically destroy the successfully initialized attr.
        auto attrDeleter = [](posix_spawnattr_t* t) {
            posix_spawnattr_destroy(t);
        };
        unique_ptr<posix_spawnattr_t, decltype(attrDeleter)> scopedAttr(
                &attr, attrDeleter);

        if (posix_spawnattr_setflags(&attr, POSIX_SPAWN_CLOEXEC_DEFAULT)) {
            LOG(VERBOSE) << "Failed to request CLOEXEC.";
            return -1;
        }

        posix_spawn_file_actions_t fileActions;
        if (posix_spawn_file_actions_init(&fileActions)) {
            LOG(VERBOSE) << "Failed to initialize fileactions obj.";
            return -1;
        }
        // Automatically destroy the successfully initialized fileActions.
        auto fileActionsDeleter = [](posix_spawn_file_actions_t* t) {
            posix_spawn_file_actions_destroy(t);
        };
        unique_ptr<posix_spawn_file_actions_t, decltype(fileActionsDeleter)>
            scopedFileActions(&fileActions, fileActionsDeleter);

        if ((options & RunOptions::DumpOutputToFile) != RunOptions::Empty) {
            if (posix_spawn_file_actions_addopen(
                        &fileActions, 1, outputFile.c_str(),
                        O_WRONLY | O_CREAT | O_TRUNC, 0700) ||
                    posix_spawn_file_actions_addopen(
                        &fileActions, 2, outputFile.c_str(),
                        O_WRONLY | O_CREAT | O_TRUNC, 0700)) {
                LOG(VERBOSE) << "Failed to redirect child output to file "
                    << outputFile;
                return -1;
            }
        } else if ((options & RunOptions::ShowOutput) != RunOptions::Empty) {
            if (posix_spawn_file_actions_addinherit_np(&fileActions, 1) ||
                    posix_spawn_file_actions_addinherit_np(&fileActions, 2)) {
                LOG(VERBOSE) << "Failed to request child stdout/stderr to be "
                    "left intact";
                return -1;
            }
        } else {
            if (posix_spawn_file_actions_addopen(&fileActions, 1, "/dev/null",
                        O_WRONLY, 0700) ||
                    posix_spawn_file_actions_addopen(&fileActions, 2, "/dev/null",
                        O_WRONLY, 0700)) {
                LOG(VERBOSE) << "Failed to redirect child output to /dev/null";
                return -1;
            }
        }

        // We never want to forward our stdin to the child process. On the other
        // hand, closing it can confuse some programs.
        if (posix_spawn_file_actions_addopen(&fileActions, 0, "/dev/null",
                    O_RDONLY, 0700)) {
            LOG(VERBOSE) << "Failed to redirect child stdin from /dev/null";
            return -1;
        }

        // Posix spawn requires that argv[0] exists.
        assert(params[0] != nullptr);

        int pid;
        if (int error_code = posix_spawnp(&pid, command, &fileActions, &attr,
                    params.data(), environ)) {
            LOG(VERBOSE) << "posix_spawnp failed: " << strerror(error_code);
            return -1;
        }
        return pid;
    }
#endif  // defined(__APPLE__)
#endif  // !_WIN32

private:
    mutable std::string mProgramDir;
    mutable std::string mLauncherDir;
    mutable std::string mHomeDir;
    mutable std::string mAppDataDir;
};

LazyInstance<HostSystem> sHostSystem = LAZY_INSTANCE_INIT;
System* sSystemForTesting = NULL;

#ifdef _WIN32
// Return |path| as a Unicode string, while discarding trailing separators.
Win32UnicodeString win32Path(StringView path) {
    Win32UnicodeString wpath(path);
    // Get rid of trailing directory separators, Windows doesn't like them.
    size_t size = wpath.size();
    while (size > 0U &&
           (wpath[size - 1U] == L'\\' || wpath[size - 1U] == L'/')) {
        size--;
    }
    if (size < wpath.size()) {
        wpath.resize(size);
    }
    return wpath;
}

using PathStat = struct _stat;

#else  // _WIN32

using PathStat = struct stat;

#endif  // _WIN32

int pathStat(StringView path, PathStat* st) {
#ifdef _WIN32
    return _wstat(win32Path(path).c_str(), st);
#else   // !_WIN32
    return HANDLE_EINTR(stat(path.c_str(), st));
#endif  // !_WIN32
}

int pathAccess(StringView path, int mode) {
#ifdef _WIN32
    // Convert |mode| to win32 permission bits.
    int win32mode = 0x0;
    if ((mode & R_OK) || (mode & X_OK)) {
        win32mode |= 0x4;
    }
    if (mode & W_OK) {
        win32mode |= 0x2;
    }
    return _waccess(win32Path(path).c_str(), win32mode);
#else   // !_WIN32
    return HANDLE_EINTR(access(path.c_str(), mode));
#endif  // !_WIN32
}

}  // namespace

// static
System* System::get() {
    System* result = sSystemForTesting;
    if (!result) {
        result = hostSystem();
    }
    return result;
}

#ifdef __x86_64__
// static
const char* System::kLibSubDir = "lib64";
// static
const char* System::kBinSubDir = "bin64";
#else
// static
const char* System::kLibSubDir = "lib";
// static
const char* System::kBinSubDir = "bin";
#endif

const char* System::kBin32SubDir = "bin";

// These need to be defined so one can take an address of them.
const int System::kProgramBitness;
const char System::kDirSeparator;
const char System::kPathSeparator;

#ifdef _WIN32
// static
const char* System::kLibrarySearchListEnvVarName = "PATH";
#elif defined(__APPLE__)
const char* System::kLibrarySearchListEnvVarName = "DYLD_LIBRARY_PATH";
#else
// static
const char* System::kLibrarySearchListEnvVarName = "LD_LIBRARY_PATH";
#endif

// static
System* System::setForTesting(System* system) {
    System* result = sSystemForTesting;
    sSystemForTesting = system;
    return result;
}

System* System::hostSystem() {
    return sHostSystem.ptr();
}

// static
std::vector<std::string> System::scanDirInternal(StringView dirPath) {
    std::vector<std::string> result;

    if (dirPath.empty()) {
        return result;
    }
#ifdef _WIN32
    auto root = PathUtils::addTrailingDirSeparator(dirPath);
    root += '*';
    Win32UnicodeString rootUnicode(root);
    struct _wfinddata_t findData;
    intptr_t findIndex = _wfindfirst(rootUnicode.c_str(), &findData);
    if (findIndex >= 0) {
        do {
            const wchar_t* name = findData.name;
            if (wcscmp(name, L".") != 0 && wcscmp(name, L"..") != 0) {
                result.push_back(Win32UnicodeString::convertToUtf8(name));
            }
        } while (_wfindnext(findIndex, &findData) >= 0);
        _findclose(findIndex);
    }
#else  // !_WIN32
    DIR* dir = ::opendir(dirPath.c_str());
    if (dir) {
        for (;;) {
            struct dirent* entry = ::readdir(dir);
            if (!entry) {
                break;
            }
            const char* name = entry->d_name;
            if (strcmp(name, ".") != 0 && strcmp(name, "..") != 0) {
                result.push_back(std::string(name));
            }
        }
        ::closedir(dir);
    }
#endif  // !_WIN32
    std::sort(result.begin(), result.end());
    return result;
}

// static
bool System::pathExistsInternal(StringView path) {
    if (path.empty()) {
        return false;
    }
    int ret = pathAccess(path, F_OK);
    return (ret == 0) || (errno != ENOENT);
}

// static
bool System::pathIsFileInternal(StringView path) {
    if (path.empty()) {
        return false;
    }
    PathStat st;
    int ret = pathStat(path, &st);
    if (ret < 0) {
        return false;
    }
    return S_ISREG(st.st_mode);
}

// static
bool System::pathIsDirInternal(StringView path) {
    if (path.empty()) {
        return false;
    }
    PathStat st;
    int ret = pathStat(path, &st);
    if (ret < 0) {
        return false;
    }
    return S_ISDIR(st.st_mode);
}

// static
bool System::pathCanReadInternal(StringView path) {
    if (path.empty()) {
        return false;
    }
    return pathAccess(path, R_OK) == 0;
}

// static
bool System::pathCanWriteInternal(StringView path) {
    if (path.empty()) {
        return false;
    }
    return pathAccess(path, W_OK) == 0;
}

// static
bool System::pathCanExecInternal(StringView path) {
    if (path.empty()) {
        return false;
    }
    return pathAccess(path, X_OK) == 0;
}

bool System::deleteFileInternal(StringView path) {
    if (!pathIsFileInternal(path)) {
        return false;
    }

    int remove_res = -1;

#ifdef _WIN32
    remove_res = remove(path.c_str());
    if (remove_res < 0) {
        // Windows sometimes just fails to delete a file
        // on the first try.
        // Sleep a little bit and try again here.
        System::get()->sleepMs(1);
        remove_res = remove(path.c_str());
    }
#else
    remove_res = remove(path.c_str());
#endif

    if (remove_res != 0) {
        LOG(VERBOSE) << "Failed to delete file [" << path << "].";
    }

    return remove_res == 0;
}

// static
bool System::pathFileSizeInternal(StringView path, FileSize* outFileSize) {
    if (path.empty() || !outFileSize) {
        return false;
    }
    PathStat st;
    int ret = pathStat(path, &st);
    if (ret < 0 || !S_ISREG(st.st_mode)) {
        return false;
    }
    // This is off_t on POSIX and a 32/64 bit integral type on windows based on
    // the host / compiler combination. We cast everything to 64 bit unsigned to
    // play safe.
    *outFileSize = static_cast<FileSize>(st.st_size);
    return true;
}

// static
Optional<System::Duration> System::pathCreationTimeInternal(StringView path) {
#if defined(__linux__) || (defined(__APPLE__) && !defined(_DARWIN_FEATURE_64_BIT_INODE))
    // TODO(zyy@): read the creation time directly from the ext4 attribute
    // on Linux.
    return {};
#else
    PathStat st;
    if (pathStat(path, &st)) {
        return {};
    }
#ifdef _WIN32
    return st.st_ctime * 1000000ll;
#else  // APPLE
    return st.st_birthtimespec.tv_sec * 1000000ll +
            st.st_birthtimespec.tv_nsec / 1000;
#endif  // WIN32 && APPLE
#endif  // Linux
}

// static
void System::addLibrarySearchDir(StringView path) {
    System* system = System::get();
    const char* varName = kLibrarySearchListEnvVarName;

    std::string libSearchPath = system->envGet(varName);
    if (libSearchPath.size()) {
        libSearchPath =
                StringFormat("%s%c%s", path, kPathSeparator, libSearchPath);
    } else {
        libSearchPath = path;
    }
    system->envSet(varName, libSearchPath);
}

// static
std::string System::findBundledExecutable(StringView programName) {
    System* const system = System::get();
    const std::string executableName = PathUtils::toExecutableName(programName);

    // first, try the root launcher directory
    std::vector<std::string> pathList = {system->getLauncherDirectory(),
                                         executableName};
    std::string executablePath = PathUtils::recompose(pathList);
    if (system->pathIsFile(executablePath)) {
        return executablePath;
    }

    // it's not there - let's try the 'bin/' subdirectory
    assert(pathList.size() == 2);
    assert(pathList[1] == executableName.c_str());
    pathList[1] = kBinSubDir;
    pathList.push_back(executableName);
    executablePath = PathUtils::recompose(pathList);
    if (system->pathIsFile(executablePath)) {
        return executablePath;
    }

#if defined(_WIN32) && defined(__x86_64)
    // On Windows we don't have a x64 version e2fsprogs, so let's try
    // 32-bit directory if 64-bit lookup failed
    assert(pathList[1] == kBinSubDir);
    pathList[1] = kBin32SubDir;
    executablePath = PathUtils::recompose(pathList);
    if (system->pathIsFile(executablePath)) {
        return executablePath;
    }
#endif

    return std::string();
}

std::string toString(OsType osType) {
    switch (osType) {
    case OsType::Windows:
        return "Windows";
    case OsType::Linux:
        return "Linux";
    case OsType::Mac:
        return "Mac";
    default:
        return "Unknown";
    }
}

}  // namespace base
}  // namespace android
