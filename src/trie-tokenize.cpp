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

typedef tdc::trie<tdc::u16string> trie_t;
typedef std::vector<std::pair<size_t, trie_t::traverse_type>> stack_t;

void traverse(const trie_t& trie, stack_t& stack, const tdc::u16string& line, size_t n) {
	if (n >= line.size()) {
		stack.push_back(make_pair(n, trie_t::traverse_type(std::numeric_limits<size_t>::max(), true)));
		std::cout << "TOKENIZED:";
		for (size_t i = 0; i < stack.size() - 1;) {
			std::cout << " ";
			size_t j = 1;
			for (; j < stack.size() - i; ++j) {
				if (stack[j].second.second == true) {
					break;
				}
			}
			utf8::utf16to8(line.begin() + stack[i].first, line.begin() + stack[i + j].first, std::ostream_iterator<char>(std::cout));
			i += j;
		}
		std::cout << std::endl;
		stack.pop_back();
		return;
	}

	size_t node = std::numeric_limits<size_t>::max();
	if (stack.back().second.second == true && stack.back().first != n) {
		node = stack.back().second.first;
	}

	trie_t::traverse_type trt = trie.traverse(line[n], node);
	if (trt.first == std::numeric_limits<size_t>::max()) {
		trt.second = true;
	}
	if (trt.second == true) {
		while (stack.back().second.second == false) {
			stack.pop_back();
		}
		stack.push_back(make_pair(n + 1, trt));
		traverse(trie, stack, line, n + 1);
		stack.pop_back();
	}
	trt.second = false;
	stack.push_back(make_pair(n, trt));
	traverse(trie, stack, line, n + 1);
}

void tokenize(std::istream& in, trie_t& trie) {
	std::string line8;
	tdc::u16string line16;

	struct state_t {
		size_t begin, end;
		trie_t::traverse_type trt;

		state_t(size_t begin = 0, size_t end = 0, trie_t::traverse_type trt = trie_t::traverse_type(std::numeric_limits<size_t>::max(), false)) :
			begin(begin),
			end(end),
			trt(trt)
		{
		}
	};
	std::vector<state_t> stack;
	std::vector<std::pair<size_t, std::string>> outputs;
	std::pair<size_t, std::string> output;

	while (std::getline(in, line8)) {
		line16.clear();
		utf8::utf8to16(line8.begin(), line8.end(), std::back_inserter(line16));

		/*
		stack_t stack;
		stack.push_back(make_pair(0, trie_t::traverse_type(std::numeric_limits<size_t>::max(), true)));
		traverse(trie, stack, line16, 0);
		//*/

		stack.clear();
		stack.resize(1);
		outputs.clear();

		bool found = false;
		do {
			found = false;
			trie_t::traverse_type trt = stack.back().trt;
			for (size_t c = stack.back().end; c < line16.size(); ++c) {
				trt = trie.traverse(line16[c], trt.first);
				if (trt.second == true || trt.first == std::numeric_limits<size_t>::max()) {
					stack.back().trt = trt;
					stack.back().end = c + 1;
					stack.push_back(state_t(c + 1));
					trt = stack.back().trt;
					found = true;
				}
			}

			while (stack.back().trt.second == false) {
				stack.pop_back();
			}
			if (stack.empty()) {
				break;
			}

			output.first = 0;
			output.second.clear();
			stack.push_back(state_t(stack.back().end, line16.size()));
			for (size_t i = 0; i < stack.size(); ++i) {
				if (stack[i].begin == stack[i].end) {
					continue;
				}
				output.second += ' ';
				if (stack[i].trt.second == false) {
					++output.first;
					output.second += '*';
				}
				utf8::utf16to8(line16.begin() + stack[i].begin, line16.begin() + stack[i].end, std::back_inserter(output.second));
			}
			outputs.push_back(output);
			std::sort(outputs.begin(), outputs.end());
			while (outputs.size() > 200) {
				outputs.pop_back();
			}
			stack.pop_back();

			if (stack.back().end == line16.size()) {
				stack.pop_back();
				while (stack.back().trt.second == false) {
					stack.pop_back();
				}
			}
		} while (found && !stack.empty());

		std::cout << "INPUT: " << line8 << std::endl;
		std::sort(outputs.begin(), outputs.end());
		for (size_t i = 0; i < outputs.size() && i<10; ++i) {
			std::cout << "TOKENIZED:" << outputs[i].second << std::endl;
		}
		std::cout << std::endl;
	}
}

int main(int argc, char *argv[]) {
	std::vector<std::string> args(argv, argv + argc);

	trie_t trie;

	if (args.size() > 1 && args[1] != "-") {
		std::ifstream in(args[1].c_str(), std::ios::binary);
		trie.unserialize(in);
	}
	else {
		trie.unserialize(std::cin);
	}

	if (args.size() > 2 && args[2] != "-") {
		std::ifstream in(args[2].c_str(), std::ios::binary);
		tokenize(in, trie);
	}
	else {
		tokenize(std::cin, trie);
	}
}
