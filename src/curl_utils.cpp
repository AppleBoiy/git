#include <curl/curl.h>
#include <iomanip>
#include <iostream>
#include <openssl/sha.h>
#include <set>
#include <string>

#include "git_utils.h"

size_t write_callback(void *receivedData, size_t elementSize, size_t numElements, void *userData) {
    size_t totalSize = elementSize * numElements;
    std::string receivedText(static_cast<char *>(receivedData), totalSize);
    auto *masterHash = static_cast<std::string *>(userData);

    if (receivedText.find("service=git-upload-pack") == std::string::npos) {
        size_t hashPos = receivedText.find("refs/heads/master\n");
        if (hashPos != std::string::npos) {
            *masterHash = receivedText.substr(hashPos - 41, 40);
        }
    }

    return totalSize;
}

size_t pack_data_callback(void *receivedData, size_t elementSize, size_t numElements, void *userData) {
    auto *accumulatedData = static_cast<std::string *>(userData);
    *accumulatedData += std::string(static_cast<char *>(receivedData), elementSize * numElements);
    return elementSize * numElements;
}

std::pair<std::string, std::string> curl_request(const std::string &url) {
    CURL *handle = curl_easy_init();
    if (!handle) {
        std::cerr << "Failed to initialize cURL.\n";
        return {};
    }

    std::string packHash;
    curl_easy_setopt(handle, CURLOPT_URL, (url + "/info/refs?service=git-upload-pack").c_str());
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &packHash);
    curl_easy_perform(handle);
    curl_easy_reset(handle);

    // Fetch git-upload-pack
    curl_easy_setopt(handle, CURLOPT_URL, (url + "/git-upload-pack").c_str());
    std::string postData = "0032want " + packHash + "\n00000009done\n";
    curl_easy_setopt(handle, CURLOPT_POSTFIELDS, postData.c_str());

    std::string packData;
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &packData);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, pack_data_callback);

    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/x-git-upload-pack-request");
    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);
    curl_easy_perform(handle);

    // Clean up
    curl_easy_cleanup(handle);
    curl_slist_free_all(headers);

    return {packData, packHash};
}
