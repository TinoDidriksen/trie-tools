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

#include <tdc_trie_tokenizer.hpp>
#include <iostream>

typedef tdc::trie_mmap<> trie_t;

int main(int argc, char *argv[]) {
	std::vector<std::string> args(argv, argv + argc);
	std::cin.sync_with_stdio(false);
	std::cout.sync_with_stdio(false);

	trie_t trie(args[1].c_str());

	tdc::trie_tokenizer<> tokenizer(trie);

	if (args.size() > 2 && args[2] != "-") {
		std::ifstream in(args[2].c_str(), std::ios::binary);
		tokenizer.tokenize(in);
	}
	else {
		tokenizer.tokenize(std::cin);
	}
}
