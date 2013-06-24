/*
* Copyright (C) 2013, Tino Didriksen Consult
* Developed by Tino Didriksen <consult@tinodidriksen.com>
*
* This file is part of trie-tools
*
* trie-tools is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* trie-tools is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with trie-tools.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef _MSC_VER
    #define _SECURE_SCL 0
    #define _CRT_SECURE_NO_DEPRECATE 1
    #define WIN32_LEAN_AND_MEAN
    #define VC_EXTRALEAN
    #define NOMINMAX
#endif

#include <tdc_trie.hpp>
#include <utf8.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

typedef tdc::trie<tdc::u16string> trie_t;

int main(int argc, char *argv[]) {
	std::vector<std::string> args(argv, argv+argc);

	trie_t trie;

	if (args.size() > 1 && args[1] != "-") {
		std::ifstream in(args[1].c_str(), std::ios::binary);
		trie.unserialize(in);
	}
	else {
		trie.unserialize(std::cin);
	}

	std::string word8;
	size_t i = 0;
	for (trie_t::const_iterator it = trie.begin() ; it != trie.end() ; ++it) {
		word8.clear();
		const tdc::u16string& word16 = *it;
		utf8::utf16to8(word16.begin(), word16.end(), std::back_inserter(word8));
		std::cout << word8 << std::endl;

		if (i % 10000 == 0) {
			std::cerr << "Printed word #" << i << " (" << word8 << ")" << std::endl;
		}
		++i;
	}
	std::cerr << "Printed " << i << " words" << std::endl;
}
