cmake_minimum_required(VERSION 3.20)
project(windows_file_sharing_validation LANGUAGES CXX)

enable_testing()

add_executable(windows-file-sharing-test windows_file_sharing_test.cpp)
target_compile_features(windows-file-sharing-test PRIVATE cxx_std_20)
add_test(NAME windows-file-sharing-test COMMAND windows-file-sharing-test)
