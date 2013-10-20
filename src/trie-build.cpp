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

#include <tdc_trie.hpp>
#include <utf8.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cctype>

typedef tdc::trie<tdc::u16string> trie_t;

void build_trie(trie_t& trie, std::istream& input) {
	std::string line8;
	tdc::u16string line16;
	size_t i=0;
	for ( ; std::getline(input, line8) ; ++i) {
		while (!line8.empty() && std::isspace(line8[line8.size()-1])) {
			line8.resize(line8.size()-1);
		}
		if (line8.empty() || line8[0] == '#') {
			continue;
		}

		line16.clear();
		utf8::utf8to16(line8.begin(), line8.end(), std::back_inserter(line16));
		trie.insert(line16);

		if (i % 10000 == 0) {
			std::cerr << "Inserted word #" << i << " (" << line8 << ")" << std::endl;
		}
	}
	std::cerr << "Inserted " << i << " words" << std::endl;
}

int main(int argc, char *argv[]) {
	std::vector<std::string> args(argv, argv+argc);

	trie_t trie;

	if (args.size() > 1 && args[1] != "-") {
		std::ifstream in(args[1].c_str(), std::ios::binary);
		build_trie(trie, in);
	}
	else {
		build_trie(trie, std::cin);
	}

	trie.compress();

	if (args.size() > 2 && args[2] != "-") {
		std::ofstream out(args[2].c_str(), std::ios::binary);
		trie.serialize(out);
	}
	else {
		trie.serialize(std::cout);
	}
}
