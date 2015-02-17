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
#include <boost/typeof/typeof.hpp>
#include <boost/foreach.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <memory>

typedef tdc::trie_mmap<> trie_t;

inline void appendJSON(std::string& str, char chr) {
	if (chr == '\n') {
		str += '\\';
		str += 'n';
	}
	else if (chr == '\\' || chr == '"') {
		str += '\\';
		str += chr;
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
		try {
			utf8::utf16to8(&chr, &chr + 1, std::back_inserter(str));
		}
		catch (utf8::invalid_utf16&) {
			str += "\xEF\xBF\xBD";
		}
	}
}

void trie_browse(const trie_t& trie, std::istream& in, std::ostream& out) {
	std::string line8, char8, buffer8(1, '{');
	tdc::u16string line16;

	while (std::getline(in, line8)) {
		std::cerr << line8 << std::endl;

		buffer8.resize(1);
		line16.clear();
		utf8::utf8to16(line8.begin(), line8.end(), std::back_inserter(line16));
		trie_t::traverse_type tt;
		for (size_t i = 0; i < line16.size(); ++i) {
			tt = trie.traverse(line16[i], tt.first);
			if (tt == trie.traverse_end()) {
				line8.clear();
				break;
			}
		}

		// If the browse prefix itself is a valid hit, output it before anything else
		if (tt.second) {
			buffer8 += '"';
			BOOST_FOREACH (char ch, line8) {
				appendJSON(buffer8, ch);
			}
			buffer8 += "\": [\"";
			BOOST_FOREACH (char ch, line8) {
				appendJSON(buffer8, ch);
			}
			buffer8 += "\"], ";
		}

		BOOST_AUTO(browser, trie.browse(tt.first));
		for (BOOST_AUTO(it, browser.begin()); it != browser.end(); ++it) {
			BOOST_AUTO(ch, *it);
			char8.clear();
			BOOST_FOREACH (char ch, line8) {
				appendJSON(char8, ch);
			}
			appendJSON(char8, ch.first);
			buffer8 += '"';
			buffer8 += char8;
			buffer8 += "\": ";
			if (ch.second <= 5) {
				char8.clear();
				BOOST_AUTO(values, it.values());
				for (BOOST_AUTO(trail, values.begin()); trail != values.end(); ++trail) {
					char8 += '"';
					BOOST_FOREACH (char ch, line8) {
						appendJSON(char8, ch);
					}
					appendJSON(char8, ch.first);
					BOOST_FOREACH (uint16_t ch, *trail) {
						appendJSON(char8, ch);
					}
					char8 += "\", ";
				}
				char8.resize(char8.size() - 2);
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
		buffer8.resize(buffer8.size()-2);
		buffer8 += '}';
		out << buffer8 << std::endl;
	}
}

int main(int argc, char *argv[]) {
	std::vector<std::string> args(argv, argv+argc);
	std::cin.sync_with_stdio(false);
	std::cout.sync_with_stdio(false);

	bool daemon = false;
	for (auto it = args.begin(); it != args.end();) {
		if (*it == "-d") {
			daemon = true;
			it = args.erase(it);
		}
		else {
			++it;
		}
	}

	trie_t trie(args[1].c_str());

	do {
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

		in->clear();
		out->clear();
		try {
			trie_browse(trie, *in, *out);
		}
		catch (std::exception& e) {
			std::cerr << "Exception caught: " << e.what() << std::endl;
			(*out) << "{}" << std::endl;
		}
		catch (...) {
			std::cerr << "Unknown exception caught on line " << __LINE__ << std::endl;
			(*out) << "{}" << std::endl;
		}
	} while (daemon);
}
