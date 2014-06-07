set(CMAKE_LEGACY_CYGWIN_WIN32 0)
cmake_minimum_required(VERSION 2.8.0 FATAL_ERROR)
project(trie-tools CXX C)

# Release or Debug
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

option(OPT_TCMALLOC "Set to OFF to disable linking against TCMalloc" ON)
if(APPLE)
	message(STATUS "Disabling TCMalloc for OS X")
	set(OPT_TCMALLOC OFF)
endif()

if(MSVC)
	set(CMAKE_CXX_FLAGS "/W4 /EHsc /MP")
	set(CMAKE_CXX_FLAGS_RELEASE "/MT /Ox /Ot /GL /GS- /DNDEBUG")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/LTCG")
	add_definitions(-DUNICODE -D_UNICODE -D_SECURE_SCL=0 -D_ITERATOR_DEBUG_LEVEL=0 -D_CRT_SECURE_NO_DEPRECATE -DWIN32_LEAN_AND_MEAN -DVC_EXTRALEAN -DNOMINMAX)
else()
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-deprecated -std=c++0x")
	set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
	include_directories("${CMAKE_CURRENT_SOURCE_DIR}/posix")

	if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
	endif()
endif()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src")
