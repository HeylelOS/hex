cmake_minimum_required(VERSION 3.14)

project(Hello LANGUAGES C)

add_executable(Hello hello.c)

install(TARGETS Hello RUNTIME)
