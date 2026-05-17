// aid-cli.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <string>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif
#include <vector>
#include <fstream>
#include <chrono>
#include <thread>
#include <atomic>
#include <cstdint>

#include <nlohmann/json.hpp>
#include "aid2/aid2.h"
#include <cstdlib>

using json = nlohmann::json;

static std::string get_config_path() {
    auto get_env_var = [](const char* name)->std::string {
#ifdef _WIN32
        char* buf = nullptr;
        size_t sz = 0;
        if (_dupenv_s(&buf, &sz, name) == 0 && buf) {
            std::string s(buf);
            free(buf);
            return s;
        }
        return std::string();
#else
        const char* v = getenv(name);
        return v ? std::string(v) : std::string();
#endif
    };
#ifdef _WIN32
    std::string home = get_env_var("USERPROFILE");
#else
    std::string home = get_env_var("HOME");
#endif
    if (home.empty()) home = ".";
    return home + "/.aid/config.json";
}

struct Config {
    std::string server;
    int64_t dsid = 0;
    bool has_dsid = false;
};

static bool load_config(Config &c) {
    std::string path = get_config_path();
    std::ifstream f(path);
    if (!f) return false;
    try {
        json j; f >> j;
        if (j.contains("server")) c.server = j["server"].get<std::string>();
        if (j.contains("dsid")) { c.dsid = j["dsid"].get<int64_t>(); c.has_dsid = true; }
        return true;
    } catch (...) { return false; }
}

static bool save_config(const Config &c) {
    std::string path = get_config_path();
    // ensure directory exists
    auto pos = path.find_last_of("/\\");
    if (pos != std::string::npos) {
        std::string dir = path.substr(0, pos);
#ifdef _WIN32
        _mkdir(dir.c_str());
#else
        mkdir(dir.c_str(), 0755);
#endif
    }
    std::ofstream f(path, std::ios::trunc);
    if (!f) return false;
    json j;
    j["server"] = c.server;
    if (c.has_dsid) j["dsid"] = c.dsid;
    f << j.dump(2);
    return true;
}

static void print_json(const json &j) {
    std::cout << j.dump() << std::endl;
}

static bool parse_arg_int64(int argc, char** argv, const std::string &name, int64_t &out) {
    for (int i = 1; i < argc; ++i) {
        std::string s = argv[i];
        // support --name value and --name=value
        if (s == name && i + 1 < argc) {
            try { out = std::stoll(argv[i+1]); return true; } catch(...) { return false; }
        }
        std::string prefix = name + "=";
        if (s.size() > prefix.size() && s.rfind(prefix, 0) == 0) {
            std::string val = s.substr(prefix.size());
            try { out = std::stoll(val); return true; } catch(...) { return false; }
        }
    }
    return false;
}

static bool parse_arg_string(int argc, char** argv, const std::string &name, std::string &out) {
    for (int i = 1; i < argc; ++i) {
        std::string s = argv[i];
        if (s == name && i + 1 < argc) {
            out = argv[i+1]; return true;
        }
        std::string prefix = name + "=";
        if (s.size() > prefix.size() && s.rfind(prefix, 0) == 0) {
            out = s.substr(prefix.size()); return true;
        }
    }
    return false;
}

static void print_general_help() {
    std::cout << "aid-cli <command> [args]\n";
    std::cout << "Commands:\n";
    std::cout << "  config set|get         Manage persistent config (server, dsid)\n";
    std::cout << "  device                 List connected devices (UDID and status)\n";
    std::cout << "  pair <udid> [--wait]   Pair device; optionally wait for trust\n";
    std::cout << "  auth <udid> [--dsid] [--server]   Authorize device (dsid precedence: CLI > config)\n";
    std::cout << "  push --dsid <n> [--server]        Push SC Info to server (dsid required)\n";
    std::cout << "  install <udid> <path> Install ipa to device\n";
    std::cout << "  service [--ipa <path>] [--auto-pair]  Start daemon loop (press Q to quit)\n";
    std::cout << "Use '<command> --help' for detailed help on a command.\n";
}

static void print_command_help(const std::string &cmd) {

    if (cmd == "config") {
        std::cout << "Usage: aid-cli config set [--server <url>] [--dsid <long_long>]\n";
        std::cout << "       aid-cli config get\n";
        std::cout << "Manage persistent configuration saved at ~/.aid/config.json\n";
        return;
    }
    if (cmd == "device") {
        std::cout << "Usage: aid-cli device\n";
        std::cout << "List connected devices: UDID and basic status.\n";
        return;
    }
    if (cmd == "pair") {
        std::cout << "Usage: aid-cli pair <udid> [--wait <s>]\n";
        std::cout << "Pair device identified by <udid>. If --wait N provided, will poll for trust up to N seconds.\n";
        return;
    }
    if (cmd == "auth") {
        std::cout << "Usage: aid-cli auth <udid> [--dsid <long_long>] [--server <url>] [--json]\n";
        std::cout << "Authorize a device. If --dsid omitted, will use dsid from config if present.\n";
        return;
    }
    if (cmd == "push") {
        std::cout << "Usage: aid-cli push --dsid <long_long> [--server <url>] [--json]\n";
        std::cout << "Push local SC Info files to the configured server. dsid is required and will not be read from config.\n";
        return;
    }
    if (cmd == "install") {
        std::cout << "Usage: aid-cli install <udid> <path> [--json]\n";
        std::cout << "Install the specified IPA to the device.\n";
        return;
    }
    if (cmd == "service") {
        std::cout << "Usage: aid-cli service [--ipa <path>] [--auto-pair]\n";
        std::cout << "Start listener loop; will wait for devices and can automate pair/auth/install when configured. Press Q to quit.\n";
        return;
    }
    std::cout << "No help available for command: " << cmd << std::endl;
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cout << "aid-cli <command> [args]\n";
        return 0;
    }

    std::string cmd = argv[1];
    Config cfg; load_config(cfg);

    bool output_json = false;
    for (int i = 1; i < argc; ++i) if (std::string(argv[i]) == "--json") output_json = true;

    // global help handling: 'aid-cli --help' or 'aid-cli <cmd> --help'
    bool helpRequested = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--help" || std::string(argv[i]) == "-h") { helpRequested = true; break; }
    }
    if (argc == 2 && helpRequested) { print_general_help(); return 0; }
    if (argc >= 3 && (std::string(argv[2]) == "--help" || std::string(argv[2]) == "-h")) { print_command_help(cmd); return 0; }

    if (cmd == "config") {
        if (argc >= 3 && std::string(argv[2]) == "set") {
            // set options
            std::string server;
            int64_t dsid = 0;
            bool has_dsid = false;
            if (parse_arg_string(argc, argv, "--server", server)) cfg.server = server;
            if (parse_arg_int64(argc, argv, "--dsid", dsid)) { cfg.dsid = dsid; cfg.has_dsid = true; }
            if (save_config(cfg)) {
                json j{{"ok",true}}; if (output_json) print_json(j); else std::cout<<"config saved\n";
                return 0;
            } else {
                json j{{"ok",false}}; if (output_json) print_json(j); else std::cout<<"save failed\n";
                return 1;
            }
        }
        if (argc >= 3 && std::string(argv[2]) == "get") {
            json j;
            j["server"] = cfg.server;
            if (cfg.has_dsid) j["dsid"] = cfg.dsid; else j["dsid"] = nullptr;
            print_json(j);
            return 0;
        }
        std::cout<<"config set|get"<<std::endl;
        return 0;
    }

    if (cmd == "auth") {
        if (argc < 3) { std::cerr<<"auth <udid> [--dsid <n>] [--server <url>]"<<std::endl; return 1; }
        std::string udid = argv[2];
        int64_t dsid = 0; bool has_dsid = false;
        if (parse_arg_int64(argc, argv, "--dsid", dsid)) has_dsid = true; else if (cfg.has_dsid) { dsid = cfg.dsid; has_dsid = true; }
        std::string server;
        if (parse_arg_string(argc, argv, "--server", server)) { Setaidserv(server.c_str()); }
        else if (!cfg.server.empty()) Setaidserv(cfg.server.c_str());

        auto status = AuthorizeDevice(udid.c_str(), has_dsid ? dsid : 0);
        if (output_json) {
            json j;
            j["udid"] = udid;
            j["status"] = (int)status;
            print_json(j);
        } else {
            std::string msg;
            switch (status) {
                case AuthorizeSuccess: msg = "Authorize: success"; break;
                case AuthorizeDopairingLocking: msg = "Authorize: device locked, disable lockscreen"; break;
                case AuthorizeDopairingTrust: msg = "Authorize: pairing requires trust (press Trust on device)"; break;
                case AuthorizeDopairingNotTrust: msg = "Authorize: user denied pairing"; break;
                case AuthorizeDopairingUnplug: msg = "Authorize: device unplugged or mux error"; break;
                case AuthorizeFailed: msg = "Authorize: failed"; break;
                default: msg = "Authorize: unknown status"; break;
            }
            std::cout << "udid: " << udid << " -> " << msg << std::endl;
        }
        return 0;
    }

    if (cmd == "push") {
        // push --dsid <n> [--server <url>]
        int64_t dsid = 0;
        if (!parse_arg_int64(argc, argv, "--dsid", dsid)) {
            std::cerr<<"push requires --dsid <long_long>"<<std::endl; return 1;
        }
        std::string server;
        if (parse_arg_string(argc, argv, "--server", server)) { Setaidserv(server.c_str()); }
        else if (!cfg.server.empty()) Setaidserv(cfg.server.c_str());

        bool ok = PushAuthorizeInfo(dsid);
        if (output_json) {
            json j{{"dsid", dsid}, {"ok", ok}};
            print_json(j);
        } else {
            if (ok) std::cout << "PushAuthorizeInfo: dsid " << dsid << " succeeded" << std::endl;
            else std::cout << "PushAuthorizeInfo: dsid " << dsid << " failed" << std::endl;
        }
        return ok ? 0 : 1;
    }

    if (cmd == "pair") {
        if (argc < 3) { std::cerr<<"pair <udid> [--wait <s>]"<<std::endl; return 1; }
        std::string udid = argv[2];
        int waitsec = 0;
        int64_t tmp=0;
        if (parse_arg_int64(argc, argv, "--wait", tmp)) waitsec = (int)tmp;

        auto try_pair = [&](std::string u)->int {
            int r = DoPair(u.c_str());
            return r;
        };

        int res = try_pair(udid);
        if (res == -19 && waitsec>0) {
            int elapsed=0;
            while (elapsed < waitsec) {
                std::this_thread::sleep_for(std::chrono::seconds(1)); elapsed++;
                res = try_pair(udid);
                if (res == 0) break;
                if (res != -19) break;
            }
        }
        if (output_json) {
            json j{{"udid", udid}, {"result", res}};
            print_json(j);
        } else {
            std::string msg;
            switch (res) {
                case 0: msg = "pairing success"; break;
                case -19: msg = "pairing dialog pending (please press Trust on device)"; break;
                case -18: msg = "pairing denied by user"; break;
                case -17: msg = "pairing failed: device locked (unlock device)"; break;
                case -8: msg = "pairing failed: usb unplugged or mux error"; break;
                default: msg = "pairing failed: unknown error"; break;
            }
            std::cout << "udid: " << udid << " -> " << msg << std::endl;
        }
        return (res==0) ? 0 : 1;
    }

    if (cmd == "install") {
        if (argc < 4) { std::cerr<<"install <udid> <path>"<<std::endl; return 1; }
        std::string udid = argv[2];
        std::string path = argv[3];
        bool ok = InstallApplication(udid.c_str(), path.c_str());
        if (output_json) {
            json j{{"udid", udid}, {"ipa", path}, {"ok", ok}};
            print_json(j);
        } else {
            std::cout << "install: udid=" << udid << " ipa=" << path << " -> " << (ok?"success":"failure") << std::endl;
        }
        return ok?0:1;
    }

    if (cmd == "service") {
        // simple service loop: start listen and wait for Q
        std::string ipa;
        bool auto_pair = false;
        if (parse_arg_string(argc, argv, "--ipa", ipa)) {}
        for (int i=1;i<argc;i++) if (std::string(argv[i])=="--auto-pair") auto_pair=true;

        if (!cfg.server.empty()) Setaidserv(cfg.server.c_str());
        StartListen();
        std::cout<<"service started. Press Q then Enter to quit."<<std::endl;
        std::atomic<bool> running(true);
        while (running) {
            std::string line;
            if (!std::getline(std::cin, line)) break;
            if (!line.empty() && (line[0]=='Q' || line[0]=='q')) {
                running = false; break;
            }
        }
        StopListen();
        return 0;
    }

    std::cout<<"unknown command"<<std::endl;
    return 1;
}

