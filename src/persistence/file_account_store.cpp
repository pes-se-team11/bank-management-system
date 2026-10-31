#include "persistence/file_account_store.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <limits>
#include <utility>

#include "persistence/record_types.h"
#include "validation/validation.h"

namespace bms::persistence {

FileAccountStore::FileAccountStore(std::string data_dir)
    : data_dir_(std::move(data_dir)),
      customers_path_(data_dir_ + "/customers.dat"),
      accounts_path_(data_dir_ + "/accounts.dat"),
      counters_path_(data_dir_ + "/counters.dat") {
    std::error_code ec;
    std::filesystem::create_directories(data_dir_, ec);

    if (ec) {
        init_status_ = Error::Io;
        return;
    }

    init_status_ = scanExistingFiles();

    if (init_status_ == Error::Ok) {
        init_status_ = loadCounters();
    }
}

Error FileAccountStore::scanExistingFiles() {
    customer_offsets_.clear();
    account_offsets_.clear();

    std::error_code ec;

    if (std::filesystem::exists(customers_path_, ec)) {
        if (ec) {
            return Error::Io;
        }

        const auto size = std::filesystem::file_size(customers_path_, ec);
        if (ec) {
            return Error::Io;
        }

        if (size % kCustomerRecordSize != 0) {
            return Error::CorruptRecord;
        }

        std::ifstream file(customers_path_, std::ios::in | std::ios::binary);
        if (!file) {
            return Error::Io;
        }

        std::array<std::uint8_t, kCustomerRecordSize> buf{};
        std::uint64_t offset = 0;

        while (file.read(
            reinterpret_cast<char*>(buf.data()),
            static_cast<std::streamsize>(buf.size()))) {

            Customer customer;
            const Error err = deserializeCustomer(buf, customer);

            if (err != Error::Ok || customer.id == 0) {
                return Error::CorruptRecord;
            }

            if (!customer_offsets_.emplace(customer.id, offset).second) {
                return Error::CorruptRecord;
            }

            if (customer.id == std::numeric_limits<CustomerId>::max()) {
                return Error::CorruptRecord;
            }

            next_customer_ = std::max(next_customer_, customer.id + 1);
            offset += kCustomerRecordSize;
        }

        if (file.bad()) {
            return Error::Io;
        }
    } else if (ec) {
        return Error::Io;
    }

    if (std::filesystem::exists(accounts_path_, ec)) {
        if (ec) {
            return Error::Io;
        }

        const auto size = std::filesystem::file_size(accounts_path_, ec);
        if (ec) {
            return Error::Io;
        }

        if (size % kAccountRecordSize != 0) {
            return Error::CorruptRecord;
        }

        std::ifstream file(accounts_path_, std::ios::in | std::ios::binary);
        if (!file) {
            return Error::Io;
        }

        std::array<std::uint8_t, kAccountRecordSize> buf{};
        std::uint64_t offset = 0;

        while (file.read(
            reinterpret_cast<char*>(buf.data()),
            static_cast<std::streamsize>(buf.size()))) {

            AccountRecord account;
            const Error err = deserializeAccount(buf, account);

            if (err != Error::Ok || account.acct_no == 0) {
                return Error::CorruptRecord;
            }

            if (!account_offsets_.emplace(account.acct_no, offset).second) {
                return Error::CorruptRecord;
            }

            if (account.acct_no == std::numeric_limits<AccountNo>::max()) {
                return Error::CorruptRecord;
            }

            next_account_ = std::max(next_account_, account.acct_no + 1);
            offset += kAccountRecordSize;
        }

        if (file.bad()) {
            return Error::Io;
        }
    } else if (ec) {
        return Error::Io;
    }

    return Error::Ok;
}

Error FileAccountStore::loadCounters() {
    std::error_code ec;

    if (!std::filesystem::exists(counters_path_, ec)) {
        if (ec) {
            return Error::Io;
        }
        return saveCounters();
    }

    const auto size = std::filesystem::file_size(counters_path_, ec);

    if (ec) {
        return Error::Io;
    }

    if (size != kCountersRecordSize) {
        return Error::CorruptRecord;
    }

    std::ifstream file(counters_path_, std::ios::in | std::ios::binary);

    if (!file) {
        return Error::Io;
    }

    std::array<std::uint8_t, kCountersRecordSize> buf{};

    file.read(
        reinterpret_cast<char*>(buf.data()),
        static_cast<std::streamsize>(buf.size())
    );

    if (!file) {
        return Error::Io;
    }

    CustomerId stored_cust = 0;
    AccountNo stored_acct = 0;

    const Error err = deserializeCounters(buf, stored_cust, stored_acct);

    if (err != Error::Ok) {
        return err;
    }

    next_customer_ = std::max(next_customer_, stored_cust);
    next_account_ = std::max(next_account_, stored_acct);

    return Error::Ok;
}

Error FileAccountStore::saveCounters() {
    std::ofstream file(
        counters_path_,
        std::ios::out | std::ios::binary | std::ios::trunc
    );

    if (!file) {
        return Error::Io;
    }

    std::array<std::uint8_t, kCountersRecordSize> buf{};
    serializeCounters(next_customer_, next_account_, buf);

    file.write(
        reinterpret_cast<const char*>(buf.data()),
        static_cast<std::streamsize>(buf.size())
    );

    file.flush();

    return file ? Error::Ok : Error::Io;
}

Error FileAccountStore::nextCustomerId(CustomerId& out) {
    if (init_status_ != Error::Ok) {
        return init_status_;
    }

    if (next_customer_ == std::numeric_limits<CustomerId>::max()) {
        return Error::InvalidInput;
    }

    out = next_customer_++;

    const Error err = saveCounters();

    if (err != Error::Ok) {
        init_status_ = err;
        return err;
    }

    return Error::Ok;
}

Error FileAccountStore::nextAccountNo(AccountNo& out) {
    if (init_status_ != Error::Ok) {
        return init_status_;
    }

    if (next_account_ == std::numeric_limits<AccountNo>::max()) {
        return Error::InvalidInput;
    }

    out = next_account_++;

    const Error err = saveCounters();

    if (err != Error::Ok) {
        init_status_ = err;
        return err;
    }

    return Error::Ok;
}

Error FileAccountStore::writeCustomer(const Customer& customer) {
    if (init_status_ != Error::Ok) {
        return init_status_;
    }

    if (customer.id == 0 ||
        customer.id == std::numeric_limits<CustomerId>::max() ||
        validation::validateText(
            customer.name, validation::kMaxName) != Error::Ok ||
        validation::validateText(
            customer.address, validation::kMaxAddress) != Error::Ok ||
        validation::validateContact(customer.contact) != Error::Ok ||
        validation::validateGovIdRef(customer.gov_id_ref) != Error::Ok) {
        return Error::InvalidInput;
    }

    std::array<std::uint8_t, kCustomerRecordSize> buf{};
    serializeCustomer(customer, buf);

    const auto it = customer_offsets_.find(customer.id);

    if (it != customer_offsets_.end()) {
        std::fstream file(
            customers_path_,
            std::ios::in | std::ios::out | std::ios::binary
        );

        if (!file) {
            return Error::Io;
        }

        file.seekp(static_cast<std::streamoff>(it->second));
        file.write(
            reinterpret_cast<const char*>(buf.data()),
            static_cast<std::streamsize>(buf.size())
        );
        file.flush();

        return file ? Error::Ok : Error::Io;
    }

    std::ofstream file(
        customers_path_,
        std::ios::out | std::ios::binary | std::ios::app
    );

    if (!file) {
        return Error::Io;
    }

    file.seekp(0, std::ios::end);
    const auto position = file.tellp();

    if (position < 0) {
        return Error::Io;
    }

    const std::uint64_t offset = static_cast<std::uint64_t>(position);

    file.write(
        reinterpret_cast<const char*>(buf.data()),
        static_cast<std::streamsize>(buf.size())
    );
    file.flush();

    if (!file) {
        return Error::Io;
    }

    customer_offsets_[customer.id] = offset;

    if (customer.id >= next_customer_) {
        next_customer_ = customer.id + 1;

        const Error err = saveCounters();
        if (err != Error::Ok) {
            init_status_ = err;
            return err;
        }
    }

    return Error::Ok;
}

Error FileAccountStore::readCustomer(CustomerId id, Customer& out) {
    if (init_status_ != Error::Ok) {
        return init_status_;
    }

    const auto it = customer_offsets_.find(id);

    if (it == customer_offsets_.end()) {
        return Error::CustomerNotFound;
    }

    std::ifstream file(customers_path_, std::ios::in | std::ios::binary);

    if (!file) {
        return Error::Io;
    }

    file.seekg(static_cast<std::streamoff>(it->second));

    std::array<std::uint8_t, kCustomerRecordSize> buf{};

    file.read(
        reinterpret_cast<char*>(buf.data()),
        static_cast<std::streamsize>(buf.size())
    );

    if (!file) {
        return Error::Io;
    }

    const Error err = deserializeCustomer(buf, out);

    if (err != Error::Ok) {
        return err;
    }

    if (out.id != id) {
        return Error::CustomerNotFound;
    }

    return Error::Ok;
}

Error FileAccountStore::createAccount(const AccountRecord& account) {
    if (init_status_ != Error::Ok) {
        return init_status_;
    }

    if (account.acct_no == 0 ||
        account.acct_no == std::numeric_limits<AccountNo>::max()) {
        return Error::InvalidInput;
    }

    if (account_offsets_.find(account.acct_no) != account_offsets_.end()) {
        return Error::InvalidInput;
    }

    std::array<std::uint8_t, kAccountRecordSize> buf{};

    const Error serialize_error = serializeAccount(account, buf);
    if (serialize_error != Error::Ok) {
        return serialize_error;
    }

    std::ofstream file(
        accounts_path_,
        std::ios::out | std::ios::binary | std::ios::app
    );

    if (!file) {
        return Error::Io;
    }

    file.seekp(0, std::ios::end);
    const auto position = file.tellp();

    if (position < 0) {
        return Error::Io;
    }

    const std::uint64_t offset = static_cast<std::uint64_t>(position);

    file.write(
        reinterpret_cast<const char*>(buf.data()),
        static_cast<std::streamsize>(buf.size())
    );
    file.flush();

    if (!file) {
        return Error::Io;
    }

    account_offsets_[account.acct_no] = offset;

    if (account.acct_no >= next_account_) {
        next_account_ = account.acct_no + 1;

        const Error err = saveCounters();
        if (err != Error::Ok) {
            init_status_ = err;
            return err;
        }
    }

    return Error::Ok;
}

Error FileAccountStore::updateAccount(const AccountRecord& account) {
    if (init_status_ != Error::Ok) {
        return init_status_;
    }

    const auto it = account_offsets_.find(account.acct_no);

    if (it == account_offsets_.end()) {
        return Error::AccountNotFound;
    }

    std::array<std::uint8_t, kAccountRecordSize> buf{};

    const Error serialize_error = serializeAccount(account, buf);
    if (serialize_error != Error::Ok) {
        return serialize_error;
    }

    std::fstream file(
        accounts_path_,
        std::ios::in | std::ios::out | std::ios::binary
    );

    if (!file) {
        return Error::Io;
    }

    file.seekp(static_cast<std::streamoff>(it->second));
    file.write(
        reinterpret_cast<const char*>(buf.data()),
        static_cast<std::streamsize>(buf.size())
    );
    file.flush();

    return file ? Error::Ok : Error::Io;
}

Error FileAccountStore::readAccount(AccountNo no, AccountRecord& out) {
    if (init_status_ != Error::Ok) {
        return init_status_;
    }

    const auto it = account_offsets_.find(no);

    if (it == account_offsets_.end()) {
        return Error::AccountNotFound;
    }

    std::ifstream file(accounts_path_, std::ios::in | std::ios::binary);

    if (!file) {
        return Error::Io;
    }

    file.seekg(static_cast<std::streamoff>(it->second));

    std::array<std::uint8_t, kAccountRecordSize> buf{};

    file.read(
        reinterpret_cast<char*>(buf.data()),
        static_cast<std::streamsize>(buf.size())
    );

    if (!file) {
        return Error::Io;
    }

    const Error err = deserializeAccount(buf, out);

    if (err != Error::Ok) {
        return err;
    }

    if (out.acct_no != no) {
        return Error::AccountNotFound;
    }

    return Error::Ok;
}

Error FileAccountStore::listAccounts(
    CustomerId id,
    std::vector<AccountRecord>& out
) {
    out.clear();

    if (init_status_ != Error::Ok) {
        return init_status_;
    }

    std::error_code ec;

    if (!std::filesystem::exists(accounts_path_, ec)) {
        return ec ? Error::Io : Error::Ok;
    }

    const auto size = std::filesystem::file_size(accounts_path_, ec);

    if (ec) {
        return Error::Io;
    }

    if (size % kAccountRecordSize != 0) {
        return Error::CorruptRecord;
    }

    std::ifstream file(accounts_path_, std::ios::in | std::ios::binary);

    if (!file) {
        return Error::Io;
    }

    std::array<std::uint8_t, kAccountRecordSize> buf{};

    while (file.read(
        reinterpret_cast<char*>(buf.data()),
        static_cast<std::streamsize>(buf.size()))) {
        AccountRecord rec;
        const Error err = deserializeAccount(buf, rec);

        if (err != Error::Ok) {
            out.clear();
            return err;
        }

        if (rec.cust_id == id) {
            out.push_back(rec);
        }
    }

    if (file.bad()) {
        out.clear();
        return Error::Io;
    }

    return Error::Ok;
}

}  // namespace bms::persistence