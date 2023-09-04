cmake_minimum_required(VERSION 3.8)

aux_source_directory(. SRCS)
add_library(v8 SHARED ${SRCS})
