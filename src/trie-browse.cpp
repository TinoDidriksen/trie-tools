/*
* Copyright (C) 2014, Tino Didriksen Consult
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
#include <memory>

typedef tdc::trie<tdc::u16string> trie_t;

inline void appendJSON(std::string& str, char chr) {
	if (chr == '\n') {
		str += '\\';
		str += 'n';
	}
	else if (chr == '\\') {
		str += '\\';
		str += '\\';
	}
	else {
		str += chr;
	}
}

inline void appendJSON(std::string& str, uint16_t chr) {
	if (chr <= 127) {
		appendJSON(str, static_cast<char>(chr));
	}
	else {
		utf8::utf16to8(&chr, &chr + 1, std::back_inserter(str));
	}
}

void trie_browse(const trie_t& trie, std::istream& in, std::ostream& out) {
	std::string line8, char8, buffer8(1, '{');
	tdc::u16string line16;
	while (std::getline(in, line8)) {
		buffer8.resize(1);
		line16.clear();
		utf8::utf8to16(line8.begin(), line8.end(), std::back_inserter(line16));
		trie_t::traverse_type tt;
		for (auto c : line16) {
			tt = trie.traverse(c, tt.first);
		}

		// If the browse prefix itself is a valid hit, output it before anything else
		if (tt.second) {
			buffer8 += '"';
			for (auto ch : line8) {
				appendJSON(buffer8, ch);
			}
			buffer8 += "\": [\"";
			for (auto ch : line8) {
				appendJSON(buffer8, ch);
			}
			buffer8 += "\"], ";
		}

		auto browser = trie.browse(tt.first);
		for (auto it = browser.begin(); it != browser.end(); ++it) {
			auto ch = *it;
			char8.clear();
			for (auto ch : line8) {
				appendJSON(char8, ch);
			}
			appendJSON(char8, ch.first);
			buffer8 += '"';
			buffer8 += char8;
			buffer8 += "\": ";
			if (ch.second <= 5) {
				char8.clear();
				for (auto trail : it.values()) {
					char8 += '"';
					for (auto ch : line8) {
						appendJSON(char8, ch);
					}
					appendJSON(char8, ch.first);
					for (auto ch : trail) {
						appendJSON(char8, ch);
					}
					char8 += "\", ";
				}
				char8.pop_back();
				char8.pop_back();
				buffer8 += '[';
				buffer8 += char8;
				buffer8 += ']';
			}
			else {
				char8.clear();
				char8.resize(12);
				char8.resize(sprintf(&char8[0], "%u", ch.second));
				buffer8 += char8;
			}
			buffer8 += ',';
			buffer8 += ' ';
		}
		buffer8.pop_back();
		buffer8.pop_back();
		buffer8 += '}';
		out << buffer8 << std::endl;
	}
}

int main(int argc, char *argv[]) {
	std::vector<std::string> args(argv, argv+argc);

	trie_t trie;
	{
		std::ifstream in_trie(args[1], std::ios::binary);
		trie.unserialize(in_trie);
	}

	std::unique_ptr<std::ifstream> in_p;
	std::unique_ptr<std::ofstream> out_p;
	std::istream *in = &std::cin;
	std::ostream *out = &std::cout;

	if (args.size() > 2 && args[2] != "-") {
		in_p.reset(new std::ifstream(args[2], std::ios::binary));
		in = in_p.get();
	}
	if (args.size() > 3 && args[3] != "-") {
		out_p.reset(new std::ofstream(args[3], std::ios::binary));
		out = out_p.get();
	}

	trie_browse(trie, *in, *out);
}
