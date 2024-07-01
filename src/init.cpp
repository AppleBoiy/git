#include "git_utils.h"
#include <filesystem>
#include <fstream>
#include <iostream>


bool init(const std::string &directory) {
    try {
        std::filesystem::create_directory(directory + "/.git");
        std::filesystem::create_directory(directory + "/.git/objects");
        std::filesystem::create_directory(directory + "/.git/refs");

        std::ofstream headFile(directory + "/.git/HEAD");
        if (headFile.is_open()) {
            headFile << "ref: refs/heads/main\n";
            headFile.close();
        } else {
            std::cerr << "Failed to create .git/HEAD file.\n";
            return false;
        }

        std::cout << "Initialized git directory\n";
    } catch (const std::filesystem::filesystem_error &e) {
        std::cerr << e.what() << '\n';
        return false;
    }
    return true;
}
