#include "git_utils.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

bool cat_file(int argc, char *argv[]) {
    try {
        if (argc <= 3) {
            std::cerr << "Invalid arguments" << "\n";
        }
        std::string flag = argv[2];
        if (flag != "-p") {
            std::cerr << "Invalid flags for cat-file, Expected '-p' " << "\n";
            return false;
        }
        std::string Hash = argv[3];
        std::string dir_name = Hash.substr(0, 2);
        std::string file_name = Hash.substr(2);
        std::string path = "./.git/objects/" + dir_name + "/" + file_name;
        auto file = std::ifstream(path);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << path << "\n";
            return false;
        }
        auto blob_data = std::string(std::istreambuf_iterator<char>(file),
                                     std::istreambuf_iterator<char>());
        std::string buf;

        if (!decompress_object(buf, blob_data)) {
            return false;
        }
        std::string trimmed_data = buf.substr(buf.find('\0') + 1);
        std::cout << trimmed_data;
    } catch (const std::filesystem::filesystem_error &e) {
        std::cerr << e.what() << '\n';
        return false;
    }
    return true;
}

