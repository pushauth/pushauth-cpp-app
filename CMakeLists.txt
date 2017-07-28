cmake_minimum_required(VERSION 3.7)
project(pushauth)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -v -stdlib=libc++")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_compile_options(-stdlib=libc++)

include_directories("libs/openssl/include")
include_directories("libs/inih-master")

include_directories("libs/curl-7.54.1 2/include/curl")

include_directories("libs/json/src")

include_directories("libs/ProgramOptions.hxx-master")


set(SOURCE_FILES main.cpp)



add_executable(pushauth ${SOURCE_FILES})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin")



target_link_libraries(${PROJECT_NAME} curl crypto)

target_link_libraries(${PROJECT_NAME} "${PROJECT_SOURCE_DIR}/libs/openssl/libcrypto.a")
