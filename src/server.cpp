#include "git_utils.h"
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "No command provided.\n";
        return EXIT_FAILURE;
    }

    std::string command = argv[1];

    if (command == "init") {
        if (!init()) {
            return EXIT_FAILURE;
        }
    } else if (command == "cat-file") {
        if (!cat_file(argc, argv)) {
            return EXIT_FAILURE;
        }
    } else if (command == "hash-object") {
        if (argc <= 3) {
            std::cerr << "Invalid Arguments\n";
            return EXIT_FAILURE;
        }
        std::string flag = argv[2];
        if (flag != "-w") {
            std::cerr << "Invalid flag for hash-object, expected '-w'\n";
            return EXIT_FAILURE;
        }
        std::string file_name = argv[3];
        std::string hash = create_blob(file_name);
        if (hash.empty()) {
            return EXIT_FAILURE;
        }
        std::cout << hash << "\n";
    } else if (command == "ls-tree") {
        if (!read_tree(argc, argv)) {
            return EXIT_FAILURE;
        }
    } else if (command == "write-tree") {
        std::string tree_hash = write_tree(".");
        if (tree_hash.empty()) {
            return EXIT_FAILURE;
        }
        std::cout << tree_hash << "\n";
    } else if (command == "commit-tree") {
        if (!commit_tree(argc, argv)) {
            return EXIT_FAILURE;
        }
    } else if (command == "clone") {
        if (!clone(argc, argv)) {
            return EXIT_FAILURE;
        }
    } else {
        std::cerr << "Unknown command: " << command << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
