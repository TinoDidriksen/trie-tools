/*
* Copyright (C) 2013-2015, Tino Didriksen <mail@tinodidriksen.com>
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

#include <tdc_trie_mmap.hpp>
#include <utf8.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

typedef tdc::trie_mmap<> trie_t;

void trie_print(const trie_t& trie, std::ostream& out) {
	size_t i = 0;
	for (trie_t::const_iterator it = trie.begin(); it != trie.end(); ++it) {
		const tdc::u16string& word16 = *it;
		utf8::utf16to8(word16.begin(), word16.end(), std::ostream_iterator<char>(out));
		out << std::endl;

		if (i % 10000 == 0) {
			std::cerr << "Printed word #" << i << std::endl;
		}
		++i;
	}
	std::cerr << "Printed " << i << " words" << std::endl;
}

int main(int argc, char *argv[]) {
	std::vector<std::string> args(argv, argv+argc);
	std::cin.sync_with_stdio(false);
	std::cout.sync_with_stdio(false);

	trie_t trie(args[1].c_str());

	if (args.size() > 2 && args[2] != "-") {
		std::ofstream out(args[2].c_str(), std::ios::binary);
		trie_print(trie, out);
	}
	else {
		trie_print(trie, std::cout);
	}
}
