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

#include <tdc_trie_tokenizer.hpp>
#include <iostream>

typedef tdc::trie<tdc::u16string> trie_t;

class apertium_printer {
private:
	std::ostream *out;
	bool first_token;

public:
	apertium_printer(std::ostream& out) :
		out(&out),
		first_token(false) {
	}

	void line_open() {
	}
	void line_close() {
		(*out) << std::endl;
	}

	void span_open() {
		(*out) << '^';
	}
	void span_print(tdc::u16string::iterator begin, tdc::u16string::iterator end) {
		utf8::utf16to8(begin, end, std::ostream_iterator<char>(*out));
	}
	void span_close() {
		(*out) << "$ ";
	}

	void tokens_open() {
		(*out) << '/';
		first_token = true;
	}
	void tokens_close() {
	}

	void token_print(tdc::u16string::iterator begin, tdc::u16string::iterator end, bool garbage = false) {
		if (!first_token) {
			(*out) << '+';
		}
		first_token = false;
		if (garbage) {
			(*out) << '*';
		}
		utf8::utf16to8(begin, end, std::ostream_iterator<char>(*out));
	}
};

int main(int argc, char *argv[]) {
	std::vector<std::string> args(argv, argv + argc);
	std::cin.sync_with_stdio(false);
	std::cout.sync_with_stdio(false);

	trie_t trie;

	if (args.size() > 1 && args[1] != "-") {
		std::ifstream in(args[1].c_str(), std::ios::binary);
		trie.unserialize(in);
	}
	else {
		trie.unserialize(std::cin);
	}

	tdc::trie_tokenizer<> tokenizer(trie);

	if (args.size() > 2 && args[2] != "-") {
		std::ifstream in(args[2].c_str(), std::ios::binary);
		tokenizer.tokenize(in, apertium_printer(std::cout));
	}
	else {
		tokenizer.tokenize(std::cin, apertium_printer(std::cout));
	}
}
