cmake_minimum_required (VERSION 3.0.0)
project (s3ls)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_library(CRYPTO_LIB NAMES crypto REQUIRED)
find_library(SSL_LIB NAMES ssl REQUIRED)
find_library(CURL_LIB NAMES curl REQUIRED)
find_library(PTHREAD_LIB NAMES pthread REQUIRED)
find_library(GFLAGS_LIB NAMES gflags REQUIRED)

add_executable (s3ls main.cc)


target_link_libraries (s3ls
    LINK_PUBLIC
    aws-cpp-sdk-s3
    aws-cpp-sdk-core
    ${CRYPTO_LIB}
    ${SSL_LIB}
    ${CURL_LIB}
    ${PTHREAD_LIB}
    ${GFLAGS_LIB}
    )

