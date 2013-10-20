# Threading, for popen_plus
find_package(Threads REQUIRED)

if(NOT MSVC)
	set(POPEN_PLUS_C ../posix/popen_plus.c ../posix/popen_plus.h)
endif()

set(UTF8 ../include/utf8.h)
set(TRIE ../include/tdc_trie.hpp)
set(TRIE_SPELL ../include/tdc_trie_speller.hpp)

add_executable(trie-build trie-build.cpp ${UTF8} ${TRIE})

add_executable(trie-print trie-print.cpp ${UTF8} ${TRIE})

add_executable(trie-spell trie-spell.cpp ${UTF8} ${TRIE} ${TRIE_SPELL})

add_executable(trie-spell-foma trie-spell-foma.cpp ${UTF8} ${TRIE} ${TRIE_SPELL} ${POPEN_PLUS_C})
target_link_libraries(trie-spell-foma ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS trie-build trie-print trie-spell trie-spell-foma RUNTIME DESTINATION bin)
