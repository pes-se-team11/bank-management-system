#include "cli/menu.h"

#include <iostream>
#include <string_view>
#include <vector>

#include "platform/platform.h"
#include "validation/validation.h"

namespace bms::cli {

namespace {

const char* typeName(AccountType type) {
    return type == AccountType::Savings ? "Savings" : "Current";
}

const char* stateName(AccountState state) {
    switch (state) {
    case AccountState::Active:
        return "Active";
    case AccountState::Locked:
        return "Locked";
    case AccountState::Closed:
        return "Closed";
    }
    return "Unknown";
}

}  // namespace

std::string formatPaise(Paise amount) {
    // Work in unsigned so the most negative value cannot overflow.
    const bool negative = amount < 0;
    const std::uint64_t magnitude =
        negative ? 0 - static_cast<std::uint64_t>(amount)
                 : static_cast<std::uint64_t>(amount);
    const std::uint64_t paise = magnitude % 100;
    std::string text = std::to_string(magnitude / 100) + '.';
    if (paise < 10) {
        text += '0';
    }
    text += std::to_string(paise);
    return negative ? '-' + text : text;
}

std::string describe(Error error) {
    switch (error) {
    case Error::Ok:
        return "Done.";
    case Error::InvalidInput:
        return "That value is not allowed. Check it and try again.";
    case Error::CustomerNotFound:
        return "No customer has that ID. Check the ID and try again.";
    case Error::AccountNotFound:
        return "No account has that number. Check the number and try again.";
    case Error::CredentialNotFound:
        return "No login exists for that ID. Ask a manager to set one up.";
    case Error::BadCredentials:
        return "Login failed: the ID or the PIN/password is wrong. Try again.";
    case Error::Io:
        return "The data files could not be written. Check disk space and "
               "folder permissions, then try again.";
    case Error::CorruptRecord:
        return "A stored record is damaged. Report this to a manager.";
    }
    return "Unexpected error. Report this to a manager.";
}

Menu::Menu(std::istream& in, std::ostream& out, AccountService& accounts,
           AuthService& auth, TransactionService& transactions,
           bool needs_first_manager)
    : in_(in),
      out_(out),
      accounts_(accounts),
      auth_(auth),
      transactions_(transactions),
      needs_first_manager_(needs_first_manager) {}

void Menu::run() {
    out_ << "=== Bank Management System ===\n";
    if (needs_first_manager_ && !setupFirstManager()) {
        return;
    }
    for (;;) {
        out_ << "\nMain menu\n"
                "  1. Staff login\n"
                "  2. Customer login\n"
                "  0. Exit\n";
        int choice = 0;
        if (!askChoice("Choice: ", choice)) {
            return;
        }
        switch (choice) {
        case 1:
            staffLogin();
            break;
        case 2:
            customerLogin();
            break;
        case 0:
            out_ << "Goodbye.\n";
            return;
        default:
            out_ << "Please choose 1, 2 or 0.\n";
        }
        if (!in_) {
            return;
        }
    }
}

bool Menu::setupFirstManager() {
    out_ << "\nFirst run: create the manager login.\n";
    for (;;) {
        std::string id;
        if (!ask("Manager staff ID: ", id)) {
            return false;
        }
        if (validation::validateStaffId(id) != Error::Ok) {
            out_ << "Staff ID must be 3-16 letters or digits.\n";
            continue;
        }
        std::string password;
        if (!askSecret("Password (8-64 characters, no spaces): ", password)) {
            return false;
        }
        const Error result =
            auth_.enrol(Principal{PrincipalKind::Staff, id}, Role::Manager, password);
        if (result == Error::Ok) {
            out_ << "Manager " << id << " created. Please log in.\n";
            needs_first_manager_ = false;
            return true;
        }
        if (result == Error::InvalidInput) {
            out_ << "Password must be 8-64 characters with no spaces or '|'.\n";
        } else {
            fail(result);
        }
    }
}

void Menu::staffLogin() {
    std::string id;
    std::string password;
    if (!ask("Staff ID: ", id) || !askSecret("Password: ", password)) {
        return;
    }
    Session session;
    const Error result =
        auth_.login(Principal{PrincipalKind::Staff, id}, password, session);
    if (result != Error::Ok) {
        fail(result);
        return;
    }
    out_ << "Welcome, " << session.operator_id << ".\n";
    staffMenu(session);
}

void Menu::customerLogin() {
    AccountNo account = 0;
    if (!askAccountNo(account)) {
        return;
    }
    std::string pin;
    if (!askSecret("PIN: ", pin)) {
        return;
    }
    Session session;
    const Error result = auth_.login(
        Principal{PrincipalKind::Customer, std::to_string(account)}, pin, session);
    if (result != Error::Ok) {
        fail(result);
        return;
    }
    out_ << "Logged in to account " << account << ".\n";
    customerMenu(session);
}

void Menu::staffMenu(const Session& session) {
    const bool manager = session.role == Role::Manager;
    for (;;) {
        out_ << "\nStaff menu (" << session.operator_id << ")\n"
                "  1. Create customer\n"
                "  2. Open account\n"
                "  3. Deposit\n"
                "  4. Withdraw\n"
                "  5. Balance enquiry\n"
                "  6. List a customer's accounts\n";
        if (manager) {
            out_ << "  7. Add staff login\n";
        }
        out_ << "  0. Logout\n";
        int choice = 0;
        if (!askChoice("Choice: ", choice)) {
            return;
        }
        AccountNo account = 0;
        switch (choice) {
        case 1:
            createCustomer();
            break;
        case 2:
            openAccount(session.operator_id);
            break;
        case 3:
            if (askAccountNo(account)) {
                deposit(session.operator_id, account);
            }
            break;
        case 4:
            if (askAccountNo(account)) {
                withdraw(session.operator_id, account);
            }
            break;
        case 5:
            if (askAccountNo(account)) {
                showBalance(account);
            }
            break;
        case 6:
            listAccounts();
            break;
        case 7:
            if (manager) {
                addStaff();
            } else {
                out_ << "Only a manager can add staff.\n";
            }
            break;
        case 0:
            out_ << "Logged out.\n";
            return;
        default:
            out_ << "Please choose one of the listed options.\n";
        }
        if (!in_) {
            return;
        }
    }
}

void Menu::customerMenu(const Session& session) {
    // A customer session's operator ID is the account number they logged in
    // with, so they can only ever act on their own account.
    std::uint32_t own = 0;
    if (validation::parseId(session.operator_id, own) != Error::Ok) {
        fail(Error::InvalidInput);
        return;
    }
    for (;;) {
        out_ << "\nCustomer menu (account " << own << ")\n"
                "  1. Balance enquiry\n"
                "  2. Deposit\n"
                "  3. Withdraw\n"
                "  0. Logout\n";
        int choice = 0;
        if (!askChoice("Choice: ", choice)) {
            return;
        }
        switch (choice) {
        case 1:
            showBalance(own);
            break;
        case 2:
            deposit(session.operator_id, own);
            break;
        case 3:
            withdraw(session.operator_id, own);
            break;
        case 0:
            out_ << "Logged out.\n";
            return;
        default:
            out_ << "Please choose 1, 2, 3 or 0.\n";
        }
        if (!in_) {
            return;
        }
    }
}

void Menu::createCustomer() {
    Customer customer;
    if (!ask("Full name: ", customer.name) || !ask("Address: ", customer.address) ||
        !ask("Contact number: ", customer.contact) ||
        !ask("Government ID reference (e.g. PAN): ", customer.gov_id_ref)) {
        return;
    }
    CustomerId id = 0;
    const Error result = accounts_.createCustomer(customer, id);
    if (result == Error::InvalidInput) {
        out_ << "Customer not created. Name up to 60 characters, address up to "
                "120, contact 7-15 digits, ID reference 4-20 letters/digits; "
                "'|' is not allowed.\n";
        return;
    }
    if (result != Error::Ok) {
        fail(result);
        return;
    }
    out_ << "Customer created. Customer ID: " << id << '\n';
}

void Menu::openAccount(const std::string& operator_id) {
    std::string text;
    if (!ask("Customer ID: ", text)) {
        return;
    }
    std::uint32_t customer = 0;
    if (validation::parseId(text, customer) != Error::Ok) {
        out_ << "Customer ID must be a positive whole number.\n";
        return;
    }
    out_ << "Account type: 1. Savings  2. Current\n";
    int type_choice = 0;
    if (!askChoice("Type: ", type_choice)) {
        return;
    }
    if (type_choice != 1 && type_choice != 2) {
        out_ << "Please choose 1 or 2.\n";
        return;
    }
    const AccountType type = type_choice == 1 ? AccountType::Savings : AccountType::Current;
    Paise opening = 0;
    if (!askAmount("Opening deposit (Rs): ", opening)) {
        return;
    }
    // The PIN is collected before the account exists, so input ending part
    // way through never leaves an account nobody can log in to (BMS-F-010).
    std::string pin;
    for (;;) {
        if (!askSecret("Set customer PIN (4-6 digits): ", pin)) {
            return;
        }
        if (validation::validatePin(pin) == Error::Ok) {
            break;
        }
        platform::secureWipe(pin.data(), pin.size());
        out_ << "PIN must be 4-6 digits.\n";
    }

    AccountNo account = 0;
    const Error result = accounts_.openAccount(customer, type, opening, operator_id, account);
    if (result != Error::Ok) {
        platform::secureWipe(pin.data(), pin.size());
        fail(result);
        return;
    }
    out_ << typeName(type) << " account opened. Account number: " << account << '\n';

    const Error enrolled = auth_.enrol(
        Principal{PrincipalKind::Customer, std::to_string(account)}, Role::Customer, pin);
    if (enrolled != Error::Ok) {
        fail(enrolled);
        return;
    }
    out_ << "PIN set.\n";
}

void Menu::addStaff() {
    std::string id;
    if (!ask("New staff ID: ", id)) {
        return;
    }
    if (validation::validateStaffId(id) != Error::Ok) {
        out_ << "Staff ID must be 3-16 letters or digits.\n";
        return;
    }
    out_ << "Role: 1. Teller  2. Manager\n";
    int role_choice = 0;
    if (!askChoice("Role: ", role_choice)) {
        return;
    }
    if (role_choice != 1 && role_choice != 2) {
        out_ << "Please choose 1 or 2.\n";
        return;
    }
    std::string password;
    if (!askSecret("Password (8-64 characters, no spaces): ", password)) {
        return;
    }
    const Role role = role_choice == 1 ? Role::Teller : Role::Manager;
    const Error result = auth_.enrol(Principal{PrincipalKind::Staff, id}, role, password);
    if (result == Error::InvalidInput) {
        out_ << "Password must be 8-64 characters with no spaces or '|'.\n";
        return;
    }
    if (result != Error::Ok) {
        fail(result);
        return;
    }
    out_ << "Staff login " << id << " created.\n";
}

void Menu::deposit(const std::string& operator_id, AccountNo account) {
    Paise amount = 0;
    if (!askAmount("Deposit amount (Rs): ", amount)) {
        return;
    }
    const Error result = transactions_.deposit(account, amount, operator_id);
    if (result == Error::InvalidInput) {
        out_ << "Deposit refused: the amount must be more than zero and within "
                "the deposit limit, and the account must be active.\n";
        return;
    }
    if (result != Error::Ok) {
        fail(result);
        return;
    }
    out_ << "Deposited Rs " << formatPaise(amount) << ".\n";
    showBalance(account);
}

void Menu::withdraw(const std::string& operator_id, AccountNo account) {
    Paise amount = 0;
    if (!askAmount("Withdraw amount (Rs): ", amount)) {
        return;
    }
    const Error result = transactions_.withdraw(account, amount, operator_id);
    if (result == Error::InvalidInput) {
        out_ << "Withdrawal refused: the amount must be more than zero and not "
                "more than the available balance, and the account must be "
                "active.\n";
        return;
    }
    if (result != Error::Ok) {
        fail(result);
        return;
    }
    out_ << "Withdrew Rs " << formatPaise(amount) << ".\n";
    showBalance(account);
}

void Menu::showBalance(AccountNo account) {
    Paise balance = 0;
    const Error result = transactions_.balance(account, balance);
    if (result != Error::Ok) {
        fail(result);
        return;
    }
    out_ << "Account " << account << " balance: Rs " << formatPaise(balance) << '\n';
}

void Menu::listAccounts() {
    std::string text;
    if (!ask("Customer ID: ", text)) {
        return;
    }
    std::uint32_t customer = 0;
    if (validation::parseId(text, customer) != Error::Ok) {
        out_ << "Customer ID must be a positive whole number.\n";
        return;
    }
    std::vector<AccountRecord> list;
    const Error result = accounts_.listAccounts(customer, list);
    if (result != Error::Ok) {
        fail(result);
        return;
    }
    if (list.empty()) {
        out_ << "This customer has no accounts.\n";
        return;
    }
    for (const AccountRecord& record : list) {
        out_ << "  " << record.acct_no << "  " << typeName(record.type) << "  "
             << stateName(record.state) << "  Rs " << formatPaise(record.balance)
             << '\n';
    }
}

bool Menu::ask(const std::string& prompt, std::string& out) {
    out_ << prompt << std::flush;
    if (!std::getline(in_, out)) {
        out_ << '\n';
        return false;
    }
    if (!out.empty() && out.back() == '\r') {
        out.pop_back();
    }
    return true;
}

bool Menu::askSecret(const std::string& prompt, std::string& out) {
    // Typed secrets are not echoed (BMS-F-014); scripted input is read as-is.
    if (&in_ != &std::cin) {
        return ask(prompt, out);
    }
    // readLineNoEcho prints the newline the hidden Enter key did not.
    out_ << prompt << std::flush;
    return platform::readLineNoEcho(out);
}

bool Menu::askChoice(const std::string& prompt, int& out) {
    std::string text;
    if (!ask(prompt, text)) {
        return false;
    }
    out = -1;
    if (text.size() == 1 && text[0] >= '0' && text[0] <= '9') {
        out = text[0] - '0';
    }
    return true;
}

bool Menu::askAccountNo(AccountNo& out) {
    std::string text;
    if (!ask("Account number: ", text)) {
        return false;
    }
    if (validation::parseId(text, out) != Error::Ok) {
        out_ << "Account number must be a positive whole number.\n";
        return false;
    }
    return true;
}

bool Menu::askAmount(const std::string& prompt, Paise& out) {
    std::string text;
    if (!ask(prompt, text)) {
        return false;
    }
    if (validation::parseAmount(text, out) != Error::Ok) {
        out_ << "Enter an amount like 500 or 500.50 (no commas or signs).\n";
        return false;
    }
    return true;
}

void Menu::fail(Error error) {
    out_ << describe(error) << '\n';
}

}  // namespace bms::cli
