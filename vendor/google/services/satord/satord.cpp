#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <algorithm>

#include <android-base/logging.h>
#include <android-base/file.h>
#include <android-base/strings.h>
#include <cutils/sockets.h>
#include <private/android_filesystem_config.h>

#define SOCKET_PATH "/dev/socket/satord"

// Ensure we only talk to System Server
bool check_peer_auth(int fd) {
    struct ucred cr;
    socklen_t len = sizeof(cr);
    if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cr, &len) == -1) return false;
    
    // Allow AID_SYSTEM (1000) or Root (0)
    if (cr.uid != 1000 && cr.uid != 0) {
        LOG(ERROR) << "AUDIT: Unauthorized connection attempt from UID " << cr.uid;
        return false;
    }
    return true;
}

bool validate_path_prefix(const std::string& path, const std::vector<std::string>& allowed) {
    char resolved[PATH_MAX];
    if (realpath(path.c_str(), resolved) == NULL) {
        // file might not exist yet for creation, so check dirname
        std::string parent = android::base::Dirname(path);
        if (realpath(parent.c_str(), resolved) == NULL) {
             LOG(ERROR) << "Failed to resolve path: " << path;
             return false;
        }
    }
    std::string safe(resolved);
    for (const auto& prefix : allowed) {
        if (safe.find(prefix) == 0) return true;
    }
    LOG(ERROR) << "Path not allowed: " << safe;
    return false;
}

bool validate_package_name(const std::string& pkg) {
    if (pkg.empty()) return false;
    for (char c : pkg) {
        if (!isalnum(c) && c != '.' && c != '_') return false;
    }
    return true;
}

// Safer alternative to system()
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

void handle_client(int client_fd) {
    if (!check_peer_auth(client_fd)) {
        close(client_fd);
        return;
    }

    uint8_t opcode;
    uint32_t len;
    
    if (read(client_fd, &opcode, 1) != 1) { close(client_fd); return; }
    if (read(client_fd, &len, 4) != 4) { close(client_fd); return; }
    
    len = ntohl(len);

    if (len > 4096) { 
        close(client_fd);
        return;
    }

    std::vector<char> buffer(len + 1);
    if (read(client_fd, buffer.data(), len) != len) { close(client_fd); return; }
    buffer[len] = 0;
    std::string payload = buffer.data();

    int result = -1;

    if (opcode == 1) { // BACKUP
        size_t null_pos = payload.find('\0');
        if (null_pos != std::string::npos) {
            std::string pkg = payload.substr(0, null_pos);
            std::string dest = payload.substr(null_pos + 1);
            
            if (validate_package_name(pkg)) {
                std::string src = "/data/data/" + pkg;
                // Note: /mnt/vendor/backup/ must be created/writable by satord
                if (validate_path_prefix(src, {"/data/data/"}) && 
                    validate_path_prefix(dest, {"/sdcard/", "/mnt/vendor/backup/"})) {
                    
                    std::vector<std::string> args = {
                        "/system/bin/tar", "-czf", dest, "-C", "/data/data", pkg, "--selinux"
                    };
                    result = secure_exec(args);
                    LOG(INFO) << "AUDIT: Backup " << pkg << " -> " << dest << " res=" << result;
                }
            }
        }
    } else if (opcode == 2) { // RESTORE
        size_t null_pos = payload.find('\0');
        if (null_pos != std::string::npos) {
             std::string pkg = payload.substr(0, null_pos);
             std::string src = payload.substr(null_pos + 1);
             
             if (validate_package_name(pkg)) {
                 std::string dest_parent = "/data/data";
                 if (validate_path_prefix(src, {"/sdcard/", "/mnt/vendor/backup/"}) &&
                     validate_path_prefix(dest_parent, {"/data/data"})) {
                     
                     // Warning: Tar overwrite is destructive.
                     // Real implementation should probably delete /data/data/pkg first
                     // to ensure clean state, but standard tar extract works too.
                     // restorecon is handled by tar --selinux if preserved, 
                     // or we must run restorecon -R separately.
                     
                     std::vector<std::string> args = {
                         "/system/bin/tar", "-xzf", src, "-C", dest_parent, "--selinux"
                     };
                     result = secure_exec(args);
                     
                     // Restorecon just in case
                     std::vector<std::string> rc_args = {
                         "/system/bin/restorecon", "-R", "/data/data/" + pkg
                     };
                     secure_exec(rc_args);
                     
                     LOG(INFO) << "AUDIT: Restore " << pkg << " from " << src << " res=" << result;
                 }
             }
        }
    } else if (opcode == 3) { // RESET
        if (payload == "1") {
            android::base::WriteStringToFile("--wipe_data\n", "/cache/recovery/command");
            result = 0;
            android::base::SetProperty("sys.powerctl", "reboot,recovery");
        }
    } else if (opcode == 4) { // SET_FLAGS
        // Payload is key=value\nkey=value...
        // Write to /cache/provisioning_flags for simplicity ensuring persistance across some reboots
        // or a vendor path.
        std::string flag_path = "/cache/recovery/provisioning_flags"; 
        if (android::base::WriteStringToFile(payload, flag_path)) {
            result = 0;
            LOG(INFO) << "AUDIT: Provisioning flags updated.";
        } else {
            result = 1;
            LOG(ERROR) << "Failed to write flags";
        }
    }

    uint32_t net_result = htonl(result);
    write(client_fd, &net_result, 4);
    close(client_fd);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_un addr;

    server_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);

    unlink(SOCKET_PATH);
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        PLOG(ERROR) << "Failed to bind " << SOCKET_PATH;
        return 1;
    }
    
    chmod(SOCKET_PATH, 0660);
    chown(SOCKET_PATH, 0, 1000); // root:system

    if (listen(server_fd, 5) == -1) {
        PLOG(ERROR) << "Failed to listen";
        return 1;
    }

    LOG(INFO) << "satord starting listening on " << SOCKET_PATH;

    while (true) {
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd > 0) handle_client(client_fd);
    }
    return 0;
}
