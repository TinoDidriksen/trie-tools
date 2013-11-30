set(CMAKE_LEGACY_CYGWIN_WIN32 0)
cmake_minimum_required(VERSION 2.8.0 FATAL_ERROR)
project(trie-tools CXX C)

# Release or Debug
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

if(MSVC)
	set(CMAKE_CXX_FLAGS "/W4 /GR- /EHsc /MP")
	set(CMAKE_CXX_FLAGS_RELEASE "/MT /Ox /Ot /GL /GS-")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/LTCG")
	add_definitions(-DUNICODE -D_UNICODE -D_SECURE_SCL=0 -D_ITERATOR_DEBUG_LEVEL=0 -D_CRT_SECURE_NO_DEPRECATE -DWIN32_LEAN_AND_MEAN -DVC_EXTRALEAN -DNOMINMAX)
else()
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-deprecated")
	set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3")
	include_directories("${CMAKE_CURRENT_SOURCE_DIR}/posix")

	# Enable C++11 if possible
	option(OPT_CXX11 "Set to OFF to disable use of C++11" ON)
	if(OPT_CXX11 AND
	((CMAKE_COMPILER_IS_GNUCXX AND NOT ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 4.6) OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 3.1)))
		message(STATUS "Enabling C++11 for ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
	endif()
	if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
	endif()
endif()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src")

option(OPT_CPACK "Set to ON to generate CPack configuration files" OFF)
if(OPT_CPACK)
	file(READ "${CMAKE_CURRENT_SOURCE_DIR}/include/tdc_trie.hpp" _trie_VERSION_FILE)
	string(REGEX REPLACE ".*TRIE_VERSION_MAJOR = ([0-9]+).*" "\\1" _trie_VERSION_MAJOR "${_trie_VERSION_FILE}")
	string(REGEX REPLACE ".*TRIE_VERSION_MINOR = ([0-9]+).*" "\\1" _trie_VERSION_MINOR "${_trie_VERSION_FILE}")
	string(REGEX REPLACE ".*TRIE_VERSION_PATCH = ([0-9]+).*" "\\1" _trie_VERSION_PATCH "${_trie_VERSION_FILE}")
	string(REGEX REPLACE ".*TRIE_REVISION = ([0-9]+).*" "\\1" _trie_REVISION "${_trie_VERSION_FILE}")
	set(VERSION "${_trie_VERSION_MAJOR}.${_trie_VERSION_MINOR}.${_trie_VERSION_PATCH}.${_trie_REVISION}")

	set(CPACK_PACKAGE_NAME "trie-tools")
	set(CPACK_PACKAGE_CONTACT "Tino Didriksen <mail@tinodidriksen.com>")
	set(CPACK_PACKAGE_VENDOR "http://tinodidriksen.com/")
	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A bunch of tools for building and using tries, primarily intended for spellers and tokenizers.")
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "libgoogle-perftools0|libgoogle-perftools4")
	set(CPACK_PACKAGE_VERSION ${VERSION})

	set(CPACK_DEBIAN_BUILD_DEPENDS debhelper cmake libgoogle-perftools-dev)
	set(CPACK_DEBIAN_PACKAGE_SOURCE_COPY svn export --force)
	set(CPACK_DEBIAN_CHANGELOG "  * Changelog is available at http://visl.sdu.dk/cg3/changelog.txt\n\n")
	execute_process(COMMAND lsb_release -is
		OUTPUT_VARIABLE _lsb_distribution OUTPUT_STRIP_TRAILING_WHITESPACE
		RESULT_VARIABLE _lsb_release_failed)
	set(CPACK_DEBIAN_DISTRIBUTION_NAME ${_lsb_distribution})
	string(TOLOWER ${CPACK_DEBIAN_DISTRIBUTION_NAME} CPACK_DEBIAN_DISTRIBUTION_NAME)
	if( ${CPACK_DEBIAN_DISTRIBUTION_NAME} STREQUAL "ubuntu" )
		set(CPACK_DEBIAN_DISTRIBUTION_RELEASES precise quantal raring saucy)
	endif()
	set(DPUT_HOST "ppa:tinodidriksen/cg3")
	include(CPack)
	include(DebSourcePPA)
endif()
