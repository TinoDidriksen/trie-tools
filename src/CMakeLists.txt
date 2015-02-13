# Threading, for popen_plus
find_package(Threads REQUIRED)

# Boost
find_path(Boost_LOCAL NAMES boost/config.hpp PATHS "${CMAKE_CURRENT_SOURCE_DIR}/../include" NO_DEFAULT_PATH)
if(Boost_LOCAL)
	set(BOOST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../include")
	find_package(Boost 1.48.0)
endif()
if(NOT Boost_FOUND)
	unset(BOOST_ROOT)
	message(STATUS "Did not find Boost locally; trying globally...")
	message(STATUS "If this doesn't work, run ./get-boost.sh and try again.")
	find_package(Boost 1.48.0 REQUIRED)
endif()
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

if(NOT MSVC)
	set(POPEN_PLUS_C ../posix/popen_plus.c ../posix/popen_plus.h)
endif()

if(NOT CMAKE_BUILD_TYPE STREQUAL Debug AND OPT_TCMALLOC)
	find_library(GOOGLE_TCMALLOC_LIB tcmalloc)
	if(GOOGLE_TCMALLOC_LIB)
		message(STATUS "Found tcmalloc: ${GOOGLE_TCMALLOC_LIB}")
		set(GOOGLE_TCMALLOC_ENABLED 1)
	else()
		message(STATUS "Can't find tcmalloc - skipping")
	endif()
endif()

macro(link_helper target)
	if(GOOGLE_TCMALLOC_ENABLED)
		target_link_libraries(${target} ${GOOGLE_TCMALLOC_LIB})
	endif()

	target_link_libraries(${target} ${CMAKE_THREAD_LIBS_INIT})
endmacro()

set(UTF8 ../include/utf8.h)
set(TRIE ../include/tdc_trie.hpp)
set(TRIE_MMAP ${TRIE} ../include/tdc_trie_mmap.hpp)
set(TRIE_SPELL ../include/tdc_trie_speller.hpp)
set(TRIE_TOKENIZE ../include/tdc_trie_tokenizer.hpp)

add_executable(trie-build trie-build.cpp ${UTF8} ${TRIE})
link_helper(trie-build)

add_executable(trie-print trie-print.cpp ${UTF8} ${TRIE_MMAP})
link_helper(trie-print)

add_executable(trie-browse trie-browse.cpp ${UTF8} ${TRIE})
link_helper(trie-browse)

add_executable(trie-tokenize trie-tokenize.cpp ${UTF8} ${TRIE} ${TRIE_TOKENIZE})
link_helper(trie-tokenize)

add_executable(trie-tokenize-apertium trie-tokenize-apertium.cpp ${UTF8} ${TRIE} ${TRIE_TOKENIZE})
link_helper(trie-tokenize-apertium)

add_executable(trie-spell trie-spell.cpp ${UTF8} ${TRIE} ${TRIE_SPELL})
link_helper(trie-spell)

add_executable(trie-spell-foma trie-spell-foma.cpp ${UTF8} ${TRIE} ${TRIE_SPELL} ${POPEN_PLUS_C})
link_helper(trie-spell-foma)

install(TARGETS
	trie-build
	trie-print
	trie-browse
	trie-tokenize
	trie-tokenize-apertium
	trie-spell
	trie-spell-foma
	RUNTIME DESTINATION bin)
