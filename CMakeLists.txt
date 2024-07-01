cmake_minimum_required(VERSION 3.13)

project(git-starter-cpp)

set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard

# Include directories
include_directories(include)

# Add source files
file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.h)

# Find and link OpenSSL
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

# Find and link Zlib
find_package(ZLIB REQUIRED)

# Find and link libcurl
find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIR})

# Add executable for the server
add_executable(server ${SOURCE_FILES})
target_link_libraries(server ${OPENSSL_LIBRARIES} ZLIB::ZLIB ${CURL_LIBRARIES})