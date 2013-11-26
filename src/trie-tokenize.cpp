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

		stack.clear();
		stack.resize(1);
		outputs.clear();

		// Build the initial smallest possible valid tokens tokenization
		size_t oc = 0;
		trie_t::traverse_type trt = stack.back().trt;
		for (size_t c = 0; c < line16.size(); ++c) {
			trt = trie.traverse(line16[c], trt.first);
			if (trt.second == true) {
				if (stack.back().end != oc) {
					stack.push_back(state_t(stack.back().end, oc));
				}
				stack.push_back(state_t(oc, c + 1, trt));
				trt.first = std::numeric_limits<size_t>::max();
				oc = c + 1;
			}
			else if (trt.first == std::numeric_limits<size_t>::max() || c == line16.size() - 1) {
				trt.first = std::numeric_limits<size_t>::max();
				c = oc;
				++oc;
			}
		}

		stack.push_back(state_t(stack.back().end, line16.size()));

		// Store a copy and try to grow a token
		bool grew = true;
		while (grew) {
			grew = false;

			// Store a copy of the current tokenization
			output.first = 0;
			output.second.clear();
			for (size_t i = 0; i < stack.size(); ++i) {
				if (stack[i].begin == stack[i].end) {
					continue;
				}
				output.second += ' ';
				if (stack[i].trt.second == false) {
					output.first += stack[i].end - stack[i].begin;
					output.second += '*';
				}
				utf8::utf16to8(line16.begin() + stack[i].begin, line16.begin() + stack[i].end, std::back_inserter(output.second));
			}
			outputs.push_back(output);

			// Try to grow a token into a subsequent invalid token
			for (size_t i = 0; i < stack.size() - 1 && !grew; ++i) {
				// Skip empty and invalid tokens
				if (stack[i].begin == stack[i].end || stack[i].trt.second == false) {
					continue;
				}
				// Won't grow in this loop if the next token is valid
				if (stack[i + 1].trt.second == true) {
					continue;
				}
				trie_t::traverse_type trt = stack[i].trt;
				for (size_t c = stack[i].end; c < line16.size(); ++c) {
					trt = trie.traverse(line16[c], trt.first);
					if (trt.second == true) {
						// Find the token that we're absorbing parts of, but bail out if it is a valid one
						size_t j = 1;
						while (stack[i + j].end < c + 1 && stack[i + j].trt.second == false) {
							++j;
						}
						// We must have hit a valid token in between, so bail out
						if (stack[i+j].end < c+1) {
							break;
						}
						stack[i].trt = trt;
						stack[i].end = c + 1;
						// Erase j-1 tokens from the stack
						for (size_t k = 0; k < j - 1; ++k) {
							stack.erase(stack.begin() + i + 1);
						}
						// The next token is where this token should now end in, so check if we are also using all of that
						if (stack[i + 1].end == c + 1) {
							stack.erase(stack.begin() + i + 1);
						}
						else {
							stack[i + 1].begin = c + 1;
						}
						grew = true;
						break;
					}
					if (trt.first == std::numeric_limits<size_t>::max() || c == line16.size() - 1) {
						break;
					}
				}
			}

			// Try to grow a token into a subsequent valid token
			for (size_t i = 0; i < stack.size() - 1 && !grew; ++i) {
				// Skip empty and invalid tokens
				if (stack[i].begin == stack[i].end || stack[i].trt.second == false) {
					continue;
				}
				trie_t::traverse_type trt = stack[i].trt;
				for (size_t c = stack[i].end; c < line16.size(); ++c) {
					trt = trie.traverse(line16[c], trt.first);
					if (trt.second == true) {
						// Find the token that we're absorbing parts of
						size_t j = 1;
						while (stack[i + j].end < c + 1) {
							++j;
						}
						stack[i].trt = trt;
						stack[i].end = c + 1;
						// Erase j-1 tokens from the stack
						for (size_t k = 0; k < j - 1; ++k) {
							stack.erase(stack.begin() + i + 1);
						}
						// The next token is where this token should now end in, so check if we are also using all of that
						if (stack[i + 1].end == c + 1) {
							stack.erase(stack.begin() + i + 1);
						}
						else {
							stack[i + 1].begin = c + 1;
							stack[i + 1].trt.second = false;
						}
						grew = true;
						break;
					}
					if (trt.first == std::numeric_limits<size_t>::max() || c == line16.size() - 1) {
						break;
					}
				}
			}
		}

		std::cout << "INPUT: " << line8 << std::endl;
		std::sort(outputs.begin(), outputs.end());
		for (size_t i = 0; i < outputs.size() && i < 10; ++i) {
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
