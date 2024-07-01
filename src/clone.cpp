#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <openssl/sha.h>
#include <sstream>
#include <string>
#include <stdexcept>

#include "git_utils.h"

int read_length(const std::string &pack, int *pos) {
    int length = pack[*pos] & 0x0F;
    if (pack[*pos] & 0x80) {
        (*pos)++;
        while (pack[*pos] & 0x80) {
            length = (length << 7) | (pack[*pos] & 0x7F);
            (*pos)++;
        }
        length = (length << 7) | pack[*pos];
    }
    (*pos)++;
    return length;
}

std::string apply_delta(const std::string &delta, const std::string &base) {
    std::string result;
    int pos = 0;
    read_length(delta, &pos);
    read_length(delta, &pos);
    while (pos < delta.size()) {
        unsigned char inst = delta[pos++];
        if (inst & 0x80) {
            int offset = 0, size = 0, offsetBytes = 0, sizeBytes = 0;
            for (int i = 3; i >= 0; i--)
                if (inst & (1 << i))
                    offset = (offset << 8) | static_cast<unsigned char>(delta[pos++]), offsetBytes++;
            for (int i = 6; i >= 4; i--)
                if (inst & (1 << i))
                    size = (size << 8) | static_cast<unsigned char>(delta[pos++]), sizeBytes++;
            result += base.substr(offset, size ? size : 0x100000);
            pos += offsetBytes + sizeBytes;
        } else {
            result += delta.substr(pos, inst & 0x7F);
            pos += inst & 0x7F;
        }
    }
    return result;
}

bool clone(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " clone <url> [<directory>]\n";
        return false;
    }
    std::string url = argv[2], dir = (argc == 4) ? argv[3] : ".";
    std::filesystem::create_directories(dir);
    if (!init(dir)) {
        std::cerr << "Failed to initialize Git repository.\n";
        return false;
    }
    auto [pack, packhash] = curl_request(url);
    if (pack.size() < 20) {
        std::cerr << "Invalid pack file size.\n";
        return false;
    }
    int num_objects = 0, pos = 20;
    for (int i = 16; i < 20; ++i) num_objects = (num_objects << 8) | (unsigned char) pack[i];
    std::string master_commit_contents;

    for (int i = 0; i < num_objects; ++i) {
        int type = (pack[pos] & 112) >> 4;
        int len = read_length(pack, &pos);
        if (type == 6) {
            std::cerr << "Offset deltas not implemented.\n";
            return false;
        } else if (type == 7) {
            std::string hash = digest_to_hash(pack.substr(pos, 20));
            pos += 20;
            std::ifstream file(dir + "/.git/objects/" + hash.insert(2, "/"), std::ios::binary);
            if (!file) {
                std::cerr << "Failed to open base object file.\n";
                return false;
            }
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string base_contents = decompress_str(buffer.str());
            std::string object_type = base_contents.substr(0, base_contents.find(' '));
            base_contents = base_contents.substr(base_contents.find('\0') + 1);
            std::string delta = decompress_str(pack.substr(pos));
            std::string reconstructed = object_type + ' ' + std::to_string(base_contents.length()) + '\0' +
                                        apply_delta(delta, base_contents);
            std::string object_hash = sha1(reconstructed);
            compress_to_file(object_hash.c_str(), reconstructed, dir);
            pos += compress_str(delta).length();
            if (hash == packhash) master_commit_contents = reconstructed.substr(reconstructed.find('\0'));
        } else {
            std::string object_contents = decompress_str(pack.substr(pos));
            pos += compress_str(object_contents).length();
            std::string object_type_str = (type == 1) ? "commit " : (type == 2) ? "tree " : "blob ";
            object_contents = object_type_str + std::to_string(object_contents.length()) + '\0' + object_contents;
            std::string object_hash = sha1(object_contents);
            compress_to_file(object_hash.c_str(), object_contents, dir);
            if (object_hash == packhash) master_commit_contents = object_contents.substr(object_contents.find('\0'));
        }
    }
    restore_tree(master_commit_contents.substr(master_commit_contents.find("tree") + 5, 40), dir, dir);
    return true;
}
