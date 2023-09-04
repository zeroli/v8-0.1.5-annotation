cmake_minimum_required(VERSION 3.8)

add_executable(shell shell.cc)

target_include_directories(shell PRIVATE ../public/)
target_link_libraries(shell PUBLIC v8)
