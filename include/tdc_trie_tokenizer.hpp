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

#pragma once
#ifndef TDC_TRIE_TOKENIZER_HPP_f28c53c53a48d38efafee7fb7004a01faaac9e22
#define TDC_TRIE_TOKENIZER_HPP_f28c53c53a48d38efafee7fb7004a01faaac9e22

#include <tdc_trie.hpp>
#include <utf8.h>
#include <fstream>
#include <vector>
#include <set>
#include <string>
#include <utility>
#include <cctype>
#include <cwctype>

namespace tdc {

	template<typename String = u16string>
	class trie_tokenizer {
	private:
		typedef trie<String> trie_t;
		const trie_t *trie_;
		std::string line8;
		u16string line16;

		typedef std::pair<size_t, size_t> token_t;
		std::vector<token_t> tokens;
		std::set<size_t> seen_tokens;
		typedef std::vector<size_t> output_t;
		output_t output;
		typedef std::vector<std::pair<size_t, output_t>> outputs_t;
		outputs_t outputs;
		size_t perfw;

		void _recurse_combos(size_t tmin, size_t tmax) {
			output.push_back(tmin);
			for (size_t to = output.back() + 1; to < tmax; ++to) {
				// If there is a perfect match and we're about to make an imperfect match, bail out...
				if (!outputs.empty() && outputs.front().first == perfw && tokens[output.back()].second != tokens[to].first) {
					break;
				}
				// Recurse this combo
				if (tokens[to].first >= tokens[output.back()].second) {
					_recurse_combos(to, tmax);
				}
			}
			// Test whether we've reached the last possible token in this combo, and save the state if so
			bool cut = true;
			for (size_t ti = output.back() + 1; ti < tmax; ++ti) {
				if (tokens[ti].first >= tokens[output.back()].second) {
					cut = false;
					break;
				}
			}
			if (cut) {
				// std::sort is ascending by default, so let high weight mean poorer quality
				size_t weight = std::numeric_limits<size_t>::max();
				for (size_t i = 0; i < output.size(); ++i) {
					seen_tokens.insert(output[i]);
					weight -= tokens[output[i]].second - tokens[output[i]].first;
				}
				// Insert the new combination in the weighted correct place to keep outputs sorted
				std::pair<size_t, output_t> ins = std::make_pair(weight, output);
				outputs_t::iterator it = std::lower_bound(outputs.begin(), outputs.end(), ins);
				outputs.insert(it, ins);
			}
			output.pop_back();
		}

	public:
		class token_printer {
		private:
			std::ostream *out;

		public:
			token_printer(std::ostream& out) :
				out(&out) {
			}

			void line_open() {
			}
			void line_close() {
				(*out) << std::endl;
			}

			void span_open() {
				(*out) << "INPUT: ";
			}
			void span_print(typename String::iterator begin, typename String::iterator end) {
				utf8::utf16to8(begin, end, std::ostream_iterator<char>(*out));
				(*out) << std::endl;
			}
			void span_close() {
			}

			void tokens_open() {
				(*out) << "TOKENS:";
			}
			void tokens_close() {
				(*out) << std::endl;
			}

			void token_open(bool garbage = false) {
				(*out) << ' ';
				if (garbage) {
					(*out) << '*';
				}
			}
			void token_print(typename String::iterator begin, typename String::iterator end) {
				utf8::utf16to8(begin, end, std::ostream_iterator<char>(*out));
			}
			void token_close() {
			}
		};

		trie_tokenizer() :
			trie_(0) {
		}

		trie_tokenizer(const trie_t& trie) :
			trie_(&trie) {
		}

		const trie_t& trie() {
			return *trie_;
		}

		void trie(const trie_t& t) {
			trie_ = &t;
		}

		template<typename Printer = token_printer>
		void tokenize(std::istream& in, Printer pout = token_printer(std::cout)) {
			if (trie_ == 0) {
				throw (-1);
			}

			while (std::getline(in, line8)) {
				// First, trim the UTF-8 since that will catch 99% of whitespace
				// Trim trailing whitespace
				while (!line8.empty() && std::isspace(line8.back())) {
					line8.resize(line8.size()-1);
				}
				// Trim leading whitespace
				for (size_t i = 0; i < line8.size(); ++i) {
					if (!std::isspace(line8[i])) {
						line8.erase(line8.begin(), line8.begin() + i); // becomes a no-op for i=0
						break;
					}
				}
				// Don't even try to tokenize empty lines...
				if (line8.empty()) {
					continue;
				}

				// Convert UTF-8 to UTF-16
				utf8::utf8to16(line8.begin(), line8.end(), std::back_inserter(line16));

				// Now trim the UTF-16 version, just to catch the 1% crazy input that uses Unicode whitespace
				// Trim trailing whitespace
				while (!line16.empty() && std::iswspace(line16.back())) {
					line16.resize(line16.size()-1);
				}
				// Trim leading whitespace
				for (size_t i = 0; i < line16.size(); ++i) {
					if (!std::iswspace(line16[i])) {
						line16.erase(line16.begin(), line16.begin() + i); // becomes a no-op for i=0
						break;
					}
				}
				// Don't even try to tokenize empty lines...
				if (line16.empty()) {
					continue;
				}

				// Reset states
				pout.line_open();
				tokens.clear();
				seen_tokens.clear();
				outputs.clear();
				line16.clear();

				// Find all possible valid tokens
				for (size_t co = 0; co < line16.size(); ++co) {
					typename trie_t::traverse_type trt(std::numeric_limits<size_t>::max(), false);
					for (size_t ci = co; ci < line16.size(); ++ci) {
						trt = trie_->traverse(line16[ci], trt.first);
						if (trt.second == true) {
							tokens.push_back(std::make_pair(co, ci + 1));
						}
						else if (trt.first == std::numeric_limits<size_t>::max()) {
							break;
						}
					}
				}

				// Special case where there are no valid tokens
				if (tokens.empty()) {
					pout.span_open();
					pout.span_print(line16.begin(), line16.end());
					pout.span_close();
					continue;
				}

				// Find the longest spans of overlapping tokens, and for each such span output all possible combinations of tokens
				size_t lastout = 0;
				for (size_t tmin = 0, tmax = 0; tmax < tokens.size(); tmin = tmax + 1, tmax = tmin) {
					outputs.clear();
					std::pair<size_t, size_t> span(tokens[tmin].first, tokens[tmax].second);
					for (size_t to = tmin; to < tokens.size(); ++to) {
						if (tokens[to].first < span.second) {
							span.second = std::max(span.second, tokens[to].second);
							tmax = to;
						}
						if (tokens[to].first >= span.second) {
							break;
						}
					}

					// Calculate the perfect weight
					perfw = std::numeric_limits<size_t>::max() - (span.second - span.first);

					// Special case where there is invalid input before any found tokens, or between spans of tokens
					if (lastout < tokens[tmin].first) {
						pout.span_open();
						pout.span_print(line16.begin() + lastout, line16.begin() + span.first);
						pout.tokens_open();
						pout.token_open(true);
						pout.token_print(line16.begin() + lastout, line16.begin() + span.first);
						pout.token_close();
						pout.tokens_close();
						pout.span_close();
					}
					lastout = span.second;

					// Build all possible combinations of tokens
					for (; tmin < tmax + 1; ++tmin) {
						// Check if this will lead to an already found output
						if (seen_tokens.count(tmin)) {
							continue;
						}

						output.clear();
						_recurse_combos(tmin, tmax + 1);
					}

					// Test whether there is one and only one perfect match, and if so write that out as invidual spans instead of sub-tokens
					if (outputs.size() > 1 && outputs[0].first == perfw && outputs[1].first != perfw) {
						const output_t& output = outputs[0].second;
						for (size_t i = 0; i < output.size(); ++i) {
							pout.span_open();
							pout.span_print(line16.begin() + tokens[output[i]].first, line16.begin() + tokens[output[i]].second);
							pout.tokens_open();
							pout.token_open();
							pout.token_print(line16.begin() + tokens[output[i]].first, line16.begin() + tokens[output[i]].second);
							pout.token_close();
							pout.tokens_close();
							pout.span_close();
						}
						continue;
					}

					// Actually output something...
					bool perfect = false;
					pout.span_open();
					pout.span_print(line16.begin() + span.first, line16.begin() + span.second);
					for (size_t i = 0; i < outputs.size(); ++i) {
						// If any perfect matches were found, stop after all perfect matches are output
						if (outputs[i].first == perfw) {
							perfect = true;
						}
						else if (perfect) {
							break;
						}

						const output_t& output = outputs[i].second;
						pout.tokens_open();
						size_t lastout = span.first;
						for (size_t j = 0; j < output.size(); ++j) {
							// Output any unclaimed characters between previous output and this token as an invalid token
							if (lastout < tokens[output[j]].first) {
								pout.token_open(true);
								pout.token_print(line16.begin() + lastout, line16.begin() + tokens[output[j]].first);
								pout.token_close();
							}
							pout.token_open();
							pout.token_print(line16.begin() + tokens[output[j]].first, line16.begin() + tokens[output[j]].second);
							pout.token_close();
							lastout = tokens[output[j]].second;
						}
						// Output any unclaimed characters between previous output and the end of the span as an invalid token
						if (lastout < span.second) {
							pout.token_open(true);
							pout.token_print(line16.begin() + lastout, line16.begin() + span.second);
							pout.token_close();
						}
						pout.tokens_close();
					}
					pout.span_close();
				}

				// Special case where there is invalid input after any found tokens
				if (lastout < line16.size()) {
					pout.span_open();
					pout.span_print(line16.begin() + lastout, line16.end());
					pout.tokens_open();
					pout.token_open(true);
					pout.token_print(line16.begin() + lastout, line16.end());
					pout.token_close();
					pout.tokens_close();
					pout.span_close();
				}
				pout.line_close();
			}
		}
	};

}

#endif
