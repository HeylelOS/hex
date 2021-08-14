cmake_minimum_required(VERSION 3.14)

project(Hex LANGUAGES C)

#########
# Build #
#########

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

find_package(Lua 5.3.0 REQUIRED)

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

target_include_directories(libhex PUBLIC include ${LUA_INCLUDE_DIR})

target_link_libraries(hex PUBLIC libhex)
target_link_libraries(libhex PUBLIC ${LUA_LIBRARIES})

set_target_properties(libhex PROPERTIES OUTPUT_NAME hex)

#################
# Documentation #
#################

find_package(Doxygen)

# We need doxygen with markdown support
if(DOXYGEN_FOUND AND DOXYGEN_VERSION VERSION_GREATER_EQUAL 1.8.0)
	set(DOXYGEN_EXTRACT_ALL YES)
	set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES)

	doxygen_add_docs(doc
		include/hex/lua.h
		docs/library/hex.md
		docs/library/hash.md
		docs/library/log.md
		docs/library/env.md
		docs/library/fs.md
		docs/rituals.md
		docs/hex.1.md
	)
endif()

###########
# Install #
###########

install(TARGETS hex libhex RUNTIME LIBRARY ARCHIVE)
install(DIRECTORY include/hex TYPE INCLUDE)

