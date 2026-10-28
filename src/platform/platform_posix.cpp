// Linux implementation of the platform layer (BMS-NF-004).
#include "platform/platform.h"

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <cerrno>
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

}  // namespace bms::platform
