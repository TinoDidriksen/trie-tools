if(NOT MSVC)
	SET(POPEN_PLUS_C ../posix/popen_plus.c)
endif()

ADD_EXECUTABLE(trie-build trie-build.cpp)

ADD_EXECUTABLE(trie-print trie-print.cpp)

ADD_EXECUTABLE(trie-spell-foma trie-spell-foma.cpp ${POPEN_PLUS_C})

install(TARGETS trie-build trie-print trie-spell-foma RUNTIME DESTINATION bin)
