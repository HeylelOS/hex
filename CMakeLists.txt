cmake_minimum_required(VERSION 3.14)

project(Hex LANGUAGES C)

#########
# Build #
#########

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

set(CMAKE_OBJCOPY_TARGET "elf64-x86-64" CACHE STRING "Output format for the objcopy command.")

include_directories(include)

find_package(Lua 5.3.0 REQUIRED)

# Compile lua runtime into lua binary
file(GLOB_RECURSE HEX_LUA_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/hex/*.lua)
add_custom_command(
	OUTPUT luac.out
	COMMAND luac -s -o ${CMAKE_CURRENT_BINARY_DIR}/luac.out ${HEX_LUA_SOURCES}
	DEPENDS ${HEX_LUA_SOURCES}
)

# Convenience to replace the path relative created symbols to fixed ones
string(REGEX REPLACE "[^A-Za-z0-9_]" "_" HEX_LUAC_OUT_SYMBOL ${CMAKE_CURRENT_BINARY_DIR}/luac.out)
# Create a binary object for the lua runtime to link it with binary
add_custom_command(
	OUTPUT luac.out.o
	COMMAND ${CMAKE_OBJCOPY} --input-target binary --output-target ${CMAKE_OBJCOPY_TARGET}
		--redefine-sym=_binary_${HEX_LUAC_OUT_SYMBOL}_start=hex_lua_runtime_start
		--redefine-sym=_binary_${HEX_LUAC_OUT_SYMBOL}_end=hex_lua_runtime_end
		--strip-symbol=_binary_${HEX_LUAC_OUT_SYMBOL}_size
		${CMAKE_CURRENT_BINARY_DIR}/luac.out ${CMAKE_CURRENT_BINARY_DIR}/luac.out.o
	DEPENDS luac.out
)

file(GLOB_RECURSE HEX_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/hex/*.c)
add_executable(hex ${HEX_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/luac.out.o)

file(GLOB_RECURSE LIBHEX_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/libhex/*.c)
add_library(libhex ${LIBHEX_SOURCES})

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

