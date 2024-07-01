#include "git_utils.h"
#include <fstream>
#include <iostream>
#include <string>

std::string create_blob(const std::string &path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << path << "\n";
        exit(EXIT_FAILURE);
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    std::string header = "blob " + std::to_string(content.size()) + '\0';
    std::string store = header + content;

    std::string blob_sha1 = sha1(store);
    std::string dir_name = blob_sha1.substr(0, 2);
    std::string file_name = blob_sha1.substr(2);
    std::string full_path = "./.git/objects/" + dir_name + "/" + file_name;

    if (system(("mkdir -p ./.git/objects/" + dir_name).c_str()) != 0) {
        std::cerr << "Failed to create directory: ./.git/objects/" + dir_name +
                     "\n";
        exit(EXIT_FAILURE);
    }

    std::string compressed_data = compress_str(store);

    std::ofstream out(full_path, std::ios::binary);
    out.write(compressed_data.c_str(), compressed_data.size());
    out.close();

    return blob_sha1;
}


