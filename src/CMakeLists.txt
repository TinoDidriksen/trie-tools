# Threading, for popen_plus
find_package(Threads REQUIRED)

if(NOT MSVC)
	set(POPEN_PLUS_C ../posix/popen_plus.c)
endif()

add_executable(trie-build trie-build.cpp)

add_executable(trie-print trie-print.cpp)

add_executable(trie-spell-foma trie-spell-foma.cpp ${POPEN_PLUS_C})
target_link_libraries(trie-spell-foma ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS trie-build trie-print trie-spell-foma RUNTIME DESTINATION bin)
