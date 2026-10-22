// Entry point. Line-based menu (v1.0) - each module adds its own entries
// in its own section of the menu layer. Business logic never lives here:
// role checks belong in the service layer (BMS-SR-007).
#include <iostream>

#include "core/version.h"

int main() {
    std::cout << "Bank Management System v" << bms::kVersion << '\n';
    return 0;
}
