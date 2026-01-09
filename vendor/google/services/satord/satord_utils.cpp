#include "satord_utils.h"

#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <algorithm>
#include <libgen.h>
#include <iostream>

#ifdef __ANDROID__
#include <android-base/logging.h>
#else
// Minimal stub for non-Android compilation (DigitalOcean tests)
#define LOG(level) std::cerr
#endif

bool validate_package_name(const std::string& pkg) {
    if (pkg.empty()) return false;
    for (char c : pkg) {
        if (!isalnum(c) && c != '.' && c != '_') return false;
    }
    return true;
}

static std::string my_dirname(const std::string& path) {
    // duplicating path because dirname might modify it
    char* buf = strdup(path.c_str());
    if (!buf) return "/";
    char* d = dirname(buf);
    std::string res(d);
    free(buf);
    return res;
}

bool validate_path_prefix(const std::string& path, const std::vector<std::string>& allowed) {
    char resolved[PATH_MAX];
    if (realpath(path.c_str(), resolved) == NULL) {
        // file might not exist yet for creation, so check dirname
        std::string parent = my_dirname(path);
        if (realpath(parent.c_str(), resolved) == NULL) {
             // Suppress log in utils to keep it quiet or generic
             // LOG(ERROR) << "Failed to resolve path: " << path;
             return false;
        }
    }
    std::string safe(resolved);
    for (const auto& prefix : allowed) {
        if (safe.find(prefix) == 0) return true;
    }
    return false;
}

int secure_exec(const std::vector<std::string>& args) {
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        // Child
        std::vector<char*> c_args;
        for (const auto& arg : args) c_args.push_back(const_cast<char*>(arg.c_str()));
        c_args.push_back(nullptr);
        
        execv(args[0].c_str(), c_args.data());
        _exit(1); // Should not reach here
    }
    int status;
    waitpid(pid, &status, 0);
    return WEXITSTATUS(status);
}
