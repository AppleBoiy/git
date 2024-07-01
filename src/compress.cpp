#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <cstdio>
#include <zlib.h>

#include "git_utils.h"

int decompress_file(FILE *input, FILE *output) {
    z_stream stream = {nullptr};
    if (inflateInit(&stream) != Z_OK) {
        std::cerr << "Failed to initialize decompression stream.\n";
        return EXIT_FAILURE;
    }

    char in[CHUNK];
    char out[CHUNK];
    bool haveHeader = false;
    char header[64];
    int ret;
    unsigned headerLen = 0, dataLen = 0;

    do {
        stream.avail_in = fread(in, 1, CHUNK, input);
        if (ferror(input)) {
            std::cerr << "Failed to read from input file.\n";
            inflateEnd(&stream);
            return EXIT_FAILURE;
        }

        if (stream.avail_in == 0) break;

        stream.next_in = reinterpret_cast<unsigned char *>(in);

        do {
            stream.avail_out = CHUNK;
            stream.next_out = reinterpret_cast<unsigned char *>(out);
            ret = inflate(&stream, Z_NO_FLUSH);

            if (ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
                std::cerr << "Decompression error: " << ret << "\n";
                inflateEnd(&stream);
                return EXIT_FAILURE;
            }

            if (!haveHeader) {
                sscanf(out, "%s %u", header, &dataLen);
                haveHeader = true;
                headerLen = strlen(out) + 1;
            }

            if (dataLen > 0) {
                if (fwrite(out + headerLen, 1, dataLen, output) != dataLen || ferror(output)) {
                    std::cerr << "Failed to write to output file.\n";
                    inflateEnd(&stream);
                    return EXIT_FAILURE;
                }
            }
        } while (stream.avail_out == 0);
    } while (ret != Z_STREAM_END);

    return inflateEnd(&stream) == Z_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}

int compress_file(FILE *input, FILE *output) {
    z_stream stream = {nullptr};
    if (deflateInit(&stream, Z_DEFAULT_COMPRESSION) != Z_OK) {
        std::cerr << "Failed to initialize compression stream.\n";
        return EXIT_FAILURE;
    }

    char in[CHUNK];
    char out[CHUNK];
    int ret;
    int flush;

    do {
        stream.avail_in = fread(in, 1, CHUNK, input);
        if (ferror(input)) {
            deflateEnd(&stream);
            std::cerr << "Failed to read from input file.\n";
            return EXIT_FAILURE;
        }

        flush = feof(input) ? Z_FINISH : Z_NO_FLUSH;
        stream.next_in = reinterpret_cast<unsigned char *>(in);

        do {
            stream.avail_out = CHUNK;
            stream.next_out = reinterpret_cast<unsigned char *>(out);
            ret = deflate(&stream, flush);

            if (ret == Z_STREAM_ERROR || ret == Z_MEM_ERROR) {
                deflateEnd(&stream);
                std::cerr << "Compression error: " << ret << "\n";
                return EXIT_FAILURE;
            }

            size_t have = CHUNK - stream.avail_out;
            if (fwrite(out, 1, have, output) != have || ferror(output)) {
                deflateEnd(&stream);
                std::cerr << "Failed to write to output file.\n";
                return EXIT_FAILURE;
            }
        } while (stream.avail_out == 0);
    } while (flush != Z_FINISH);

    return deflateEnd(&stream) == Z_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}


void compress_to_file(const std::string &hash, const std::string &content,
                      const std::string &dir) {
    FILE *input = fmemopen((void *) content.c_str(), content.length(), "rb");
    std::string hash_folder = hash.substr(0, 2);
    std::string object_path = dir + "/.git/objects/" + hash_folder + '/';
    if (!std::filesystem::exists(object_path)) {
        std::filesystem::create_directories(object_path);
    }

    std::string object_file_path = object_path + hash.substr(2, 38);
    if (!std::filesystem::exists(object_file_path)) {
        FILE *output = fopen(object_file_path.c_str(), "wb");
        if (compress_file(input, output) != EXIT_SUCCESS) {
            std::cerr << "Failed to compress_file data.\n";
            return;
        }
        fclose(output);
    }
    fclose(input);
}

bool decompress_object(std::string &buf, std::string data) {
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.avail_in = 0;
    stream.next_in = Z_NULL;
    int ret = inflateInit(&stream);
    if (ret != Z_OK) {
        return false;
    }
    stream.avail_in = data.size();
    stream.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(data.data()));
    std::string out;
    char outbuffer[32768];
    do {
        stream.avail_out = 32768;
        stream.next_out = reinterpret_cast<Bytef *>(outbuffer);
        ret = inflate(&stream, Z_NO_FLUSH);
        if (ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
            inflateEnd(&stream);
            return false;
        }
        out.append(outbuffer, 32768 - stream.avail_out);
    } while (ret != Z_STREAM_END);
    inflateEnd(&stream);
    buf = out;
    return true;
}

std::string decompress_str(const std::string &compressed_str) {
    std::string decompressed_str;
    if (!decompress_object(decompressed_str, compressed_str)) {
        throw std::runtime_error("Failed to decompress_file string");
    }
    return decompressed_str;
}


std::string compress_str(const std::string &str) {
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;

    int ret = deflateInit(&stream, Z_BEST_COMPRESSION);
    if (ret != Z_OK) {
        throw std::runtime_error("Failed to initialize compression stream");
    }

    stream.avail_in = str.size();
    stream.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(str.data()));

    std::string out;
    char outbuffer[32768];

    do {
        stream.avail_out = sizeof(outbuffer);
        stream.next_out = reinterpret_cast<Bytef *>(outbuffer);

        ret = deflate(&stream, Z_FINISH);
        if (ret == Z_STREAM_ERROR) {
            deflateEnd(&stream);
            throw std::runtime_error("Failed to compress string");
        }

        out.append(outbuffer, sizeof(outbuffer) - stream.avail_out);
    } while (ret != Z_STREAM_END);

    deflateEnd(&stream);
    return out;
}