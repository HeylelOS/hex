cmake_minimum_required(VERSION 3.14)

project(Hex LANGUAGES C)

#########
# Build #
#########

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

find_package(Lua 5.3.0 REQUIRED)

include_directories(include ${LUA_INCLUDE_DIR})

# Compiling tools

file(GLOB_RECURSE BIN2SRC_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/tools/bin2src/*.c)
add_executable(bin2src ${BIN2SRC_SOURCES})

# Compile lua runtime into lua binary and then encode to C source file
# creating the bin2src tool to handle embedding data was the most portable way I found
# as we're not guaranteed on the behaviour of each platform's linker nor the presence of objcopy.

file(GLOB_RECURSE LIBHEX_LUA_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/libhex/*.lua)

add_custom_command(
	OUTPUT luac.out
	COMMAND luac -s -o ${PROJECT_BINARY_DIR}/luac.out ${LIBHEX_LUA_SOURCES}
	DEPENDS ${LIBHEX_LUA_SOURCES}
)

add_custom_command(
	OUTPUT luac.out.c
	COMMAND bin2src -S hex_runtime -o ${PROJECT_BINARY_DIR}/luac.out.c -- ${PROJECT_BINARY_DIR}/luac.out
	DEPENDS ${PROJECT_BINARY_DIR}/luac.out
)

# Compile executable and library

file(GLOB_RECURSE HEX_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/hex/*.c)
add_executable(hex ${HEX_SOURCES})

file(GLOB_RECURSE LIBHEX_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/libhex/*.c)
add_library(libhex ${LIBHEX_SOURCES} ${PROJECT_BINARY_DIR}/luac.out.c)

target_link_libraries(hex PUBLIC libhex)
target_link_libraries(libhex PUBLIC ${LUA_LIBRARIES})

set_target_properties(libhex PROPERTIES
	OUTPUT_NAME hex
	PUBLIC_HEADER include/hex/lua.h
)

###########
# Install #
###########

install(
	TARGETS hex libhex
	RUNTIME LIBRARY ARCHIVE PUBLIC_HEADER
)

