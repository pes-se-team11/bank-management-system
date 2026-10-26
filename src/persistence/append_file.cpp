#include "persistence/append_file.h"

#include <filesystem>
#include <fstream>
#include <utility>

namespace bms::persistence {

Error appendLine(const std::string& path, const std::string& line) {
    // BMS-F-060, BMS-SR-005: never open a writable journal for overwrite.
    std::ofstream file(path, std::ios::app);
    if (!file) {
        return Error::Io;
    }
    file << line << '\n';
    file.flush();
    if (!file) {
        return Error::Io;
    }
    file.close();
    return file ? Error::Ok : Error::Io;
}

Error readLines(const std::string& path, std::vector<std::string>& out) {
    out.clear();
    std::error_code error;
    const bool exists = std::filesystem::exists(path, error);
    if (error) {
        return Error::Io;
    }
    if (!exists) {
        return Error::Ok;
    }
    std::ifstream file(path);
    if (!file) {
        return Error::Io;
    }
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        if (file.eof()) {
            return Error::CorruptRecord;  // BMS-SR-005: incomplete final write
        }
        lines.push_back(line);
    }
    if (file.bad() || !file.eof()) {
        return Error::Io;
    }
    out = std::move(lines);
    return Error::Ok;
}

}  // namespace bms::persistence
