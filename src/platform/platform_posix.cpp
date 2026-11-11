// Linux implementation of the platform layer (BMS-NF-004).
#include "platform/platform.h"

#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <iostream>

namespace bms::platform {

bool secureRandom(std::uint8_t* buf, std::size_t len) {
    const int fd = ::open("/dev/urandom", O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        return false;
    }
    std::size_t got = 0;
    while (got < len) {
        const ssize_t n = ::read(fd, buf + got, len - got);
        if (n < 0 && errno == EINTR) {
            continue;
        }
        if (n <= 0) {
            ::close(fd);
            return false;
        }
        got += static_cast<std::size_t>(n);
    }
    ::close(fd);
    return true;
}

bool readLineNoEcho(std::string& out) {
    termios saved{};
    const bool terminal =
        ::isatty(STDIN_FILENO) != 0 && ::tcgetattr(STDIN_FILENO, &saved) == 0;
    if (terminal) {
        termios quiet = saved;
        quiet.c_lflag &= ~static_cast<tcflag_t>(ECHO);
        ::tcsetattr(STDIN_FILENO, TCSAFLUSH, &quiet);
    }
    const bool ok = static_cast<bool>(std::getline(std::cin, out));
    if (terminal) {
        ::tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved);
        std::cout << '\n';  // the user's Enter key was not echoed either
    }
    return ok;
}

void secureWipe(void* data, std::size_t len) {
    // Writes through a volatile pointer cannot be removed as "dead stores".
    volatile unsigned char* p = static_cast<volatile unsigned char*>(data);
    for (std::size_t i = 0; i < len; ++i) {
        p[i] = 0;
    }
}

bool secureDataDirectory(const std::string& dir, std::string& reason) {
    // Every file created from here on is owner-only (mode 0600 / 0700).
    ::umask(S_IRWXG | S_IRWXO);

    struct stat info{};
    if (::stat(dir.c_str(), &info) != 0) {
        if (::mkdir(dir.c_str(), 0700) != 0 && errno != EEXIST) {
            reason = "Could not create the data folder '" + dir + "'.";
            return false;
        }
        if (::stat(dir.c_str(), &info) != 0) {
            reason = "Could not read the data folder '" + dir + "'.";
            return false;
        }
    }
    if (!S_ISDIR(info.st_mode)) {
        reason = "'" + dir + "' exists but is not a folder.";
        return false;
    }
    if ((info.st_mode & (S_IWGRP | S_IWOTH)) != 0) {
        reason = "The data folder '" + dir +
                 "' can be changed by other users. Run: chmod 700 " + dir;
        return false;
    }
    return true;
}

InstanceLock::~InstanceLock() {
    if (handle_ >= 0) {
        ::close(static_cast<int>(handle_));  // closing drops the flock
    }
}

bool InstanceLock::acquire(const std::string& path) {
    if (handle_ >= 0) {
        return true;
    }
    const int fd = ::open(path.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0600);
    if (fd < 0) {
        return false;
    }
    if (::flock(fd, LOCK_EX | LOCK_NB) != 0) {
        ::close(fd);
        return false;
    }
    handle_ = fd;
    return true;
}

bool replaceFileDurably(const std::string& tmp, const std::string& dest) {
    const int fd = ::open(tmp.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        return false;
    }
    const bool synced = ::fsync(fd) == 0;
    ::close(fd);
    if (!synced || std::rename(tmp.c_str(), dest.c_str()) != 0) {
        return false;
    }
    // Make the rename itself durable. Best effort: the data is already safe.
    std::string parent = std::filesystem::path(dest).parent_path().string();
    if (parent.empty()) {
        parent = ".";
    }
    const int dfd = ::open(parent.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (dfd >= 0) {
        (void)::fsync(dfd);
        ::close(dfd);
    }
    return true;
}

}  // namespace bms::platform
