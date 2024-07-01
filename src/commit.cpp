#include "git_utils.h"
#include <iomanip>
#include <iostream>
#include <openssl/sha.h>
#include <sstream>
#include <vector>
#include <fstream>
#include <zlib.h>

bool commit_tree(int argc, char *argv[]) {
    if (argc < 6) {
        std::cerr << "Invalid arguments\n";
        return false;
    }

    std::string tree_sha = argv[2];
    std::string parent_flag = argv[3];
    std::string parent_sha = argv[4];
    std::string message_flag = argv[5];
    std::string commit_message = argv[6];

    if (parent_flag != "-p" || message_flag != "-m") {
        std::cerr << "Expected flags: -p <parent_commit_sha> -m <message>\n";
        return false;
    }

    std::string commit_content = "tree " + tree_sha + "\n";
    commit_content += "parent " + parent_sha + "\n";
    commit_content += "author Example <example@example.com> 0 +0000\n";
    commit_content += "committer Example <example@example.com> 0 +0000\n\n";
    commit_content += commit_message + "\n";

    std::string header = "commit " + std::to_string(commit_content.size()) + '\0';
    std::string store = header + commit_content;

    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char *>(store.c_str()), store.size(),
         hash);
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (unsigned char i: hash) {
        ss << std::setw(2) << static_cast<int>(i);
    }
    std::string commit_sha1 = ss.str();

    std::string dir_name = commit_sha1.substr(0, 2);
    std::string file_name = commit_sha1.substr(2);
    std::string path = "./.git/objects/" + dir_name;
    if (system(("mkdir -p " + path).c_str()) != 0) {
        std::cerr << "Failed to create directory: " << path << "\n";
        return false;
    }
    path += "/" + file_name;

    uLongf compressed_size = compressBound(store.size());
    std::vector<char> compressed_data(compressed_size);
    int res = compress(
            reinterpret_cast<Bytef *>(compressed_data.data()), &compressed_size,
            reinterpret_cast<const Bytef *>(store.data()), store.size());
    if (res != Z_OK) {
        std::cerr << "Failed to compress_file data, code: " << res << "\n";
        return false;
    }
    compressed_data.resize(compressed_size);

    std::ofstream out(path, std::ios::binary);
    out.write(compressed_data.data(), compressed_data.size());
    out.close();

    std::cout << commit_sha1 << "\n";
    return true;
}
