cmake_minimum_required(VERSION 3.14)

project(cmake-hello LANGUAGES C)

add_executable(cmake-hello cmake-hello.c)

install(TARGETS cmake-hello RUNTIME)
