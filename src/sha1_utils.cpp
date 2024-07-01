#include "git_utils.h"
#include <iomanip>
#include <openssl/sha.h>
#include <sstream>

std::string hex_to_binary(const std::string &hex) {
    std::string binary;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byte_string = hex.substr(i, 2);
        char byte = static_cast<char>(std::stoi(byte_string, nullptr, 16));
        binary.push_back(byte);
    }
    return binary;
}

std::string digest_to_hash(const std::string &digest) {
    std::stringstream ss;
    for (unsigned char c: digest) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
    }
    return ss.str();
}

std::string sha1(const std::string &data) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char *>(data.c_str()), data.size(),
         hash);
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (unsigned char i: hash) {
        ss << std::setw(2) << static_cast<int>(i);
    }
    return ss.str();
}
