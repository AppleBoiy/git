#include "git_utils.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>

bool read_tree(int argc, char *argv[]) {
    std::string flag;
    std::string tree_sha;
    if (argc <= 2) {
        std::cerr << "Invalid Arguments" << "\n";
        return false;
    }
    if (argc == 3) {
        tree_sha = argv[2];
    } else {
        tree_sha = argv[3];
        flag = argv[2];
    }
    if (!flag.empty() && flag != "--name-only") {
        std::cerr << "Incorrect flag :" << flag << "\nexpected --name-only" << "\n";
        return false;
    }
    std::string dir_name = tree_sha.substr(0, 2);
    std::string file_name = tree_sha.substr(2);
    std::string path = "./.git/objects/" + dir_name + "/" + file_name;
    auto file = std::ifstream(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << path << "\n";
        return false;
    }
    auto tree_data = std::string(std::istreambuf_iterator<char>(file),
                                 std::istreambuf_iterator<char>());
    std::string buf;
    if (!decompress_object(buf, tree_data)) {
        return false;
    }
    std::string trimmed_data = buf.substr(buf.find('\0') + 1);
    std::string line;
    std::vector < std::string > names;
    do {
        line = trimmed_data.substr(0, trimmed_data.find('\0'));
        if (line.substr(0, 5) == "40000")
            names.push_back(line.substr(6));
        else
            names.push_back(line.substr(7));
        trimmed_data = trimmed_data.substr(trimmed_data.find('\0') + 21);
    } while (trimmed_data.size() > 1);
    sort(names.begin(), names.end());
    for (const auto &name: names) {
        std::cout << name << "\n";
    }
    return true;
}

std::string write_tree(const std::string &dest) {
    std::vector<std::pair<std::string, std::string>> entries;

    for (const auto &entry: std::filesystem::directory_iterator(dest)) {
        std::string name = entry.path().filename().string();
        if (name == ".git") continue;

        std::string full_path = entry.path().string();
        std::string mode = std::filesystem::is_directory(entry.status()) ? "40000" : "100644";
        std::string sha1_hash = std::filesystem::is_directory(entry.status()) ? write_tree(full_path) : create_blob(
                full_path);

        if (!sha1_hash.empty()) {
            entries.emplace_back(name, std::string{mode} + " " + name + '\0' + hex_to_binary(sha1_hash));
        }
    }

    std::sort(entries.begin(), entries.end());

    std::string tree_content;
    for (const auto &entry: entries) {
        tree_content += entry.second;
    }

    std::string store = "tree " + std::to_string(tree_content.size()) + '\0' + tree_content;
    std::string tree_sha1 = sha1(store);
    std::string obj_path = "./.git/objects/" + tree_sha1.substr(0, 2) + "/" + tree_sha1.substr(2);

    if (system(("mkdir -p " + obj_path.substr(0, obj_path.find_last_of('/'))).c_str()) != 0) {
        std::cerr << "Failed to create directory: " << obj_path << "\n";
        exit(EXIT_FAILURE);
    }

    std::ofstream(obj_path, std::ios::binary) << compress_str(store);
    return tree_sha1;
}

void restore_tree(const std::string &hash, const std::string &dest, const std::string &repo_dir) {
    std::string object_path = repo_dir + "/.git/objects/" + hash.substr(0, 2) + '/' + hash.substr(2);
    std::ifstream master_tree(object_path);
    std::ostringstream buffer;
    buffer << master_tree.rdbuf();
    std::string decompressed = decompress_str(buffer.str());
    std::string tree_contents = decompressed.substr(decompressed.find('\0') + 1);

    std::string::size_type pos = 0;

    while (pos < tree_contents.length()) {
        bool is_dir = tree_contents.find("40000", pos) == pos;
        pos += is_dir ? 6 : 7;
        std::string::size_type path_start = pos;
        std::string::size_type path_end = tree_contents.find('\0', pos);
        std::string path = tree_contents.substr(path_start, path_end - path_start);
        pos = path_end + 1;
        std::string next_hash = digest_to_hash(tree_contents.substr(pos, 20));
        std::string file_path = std::string{dest} + '/' + path;

        if (is_dir) {
            std::filesystem::create_directory(file_path);
            restore_tree(next_hash, file_path, repo_dir);
        } else {
            FILE *new_file = fopen(file_path.c_str(), "wb");

            std::string blob_path = repo_dir + "/.git/objects/";
            blob_path += next_hash.substr(0, 2) + '/';
            blob_path += next_hash.substr(2);

            FILE *blob_file = fopen(blob_path.c_str(), "rb");
            if (!blob_file) throw std::runtime_error("Invalid object hash.");

            decompress_file(blob_file, new_file);
            fclose(blob_file);
            fclose(new_file);
        }

        pos += 20; // move to the next hash
    }
}


