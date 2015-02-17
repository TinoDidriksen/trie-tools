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

#pragma once
#ifndef TDC_TRIE_TOKENIZER_HPP_f28c53c53a48d38efafee7fb7004a01faaac9e22
#define TDC_TRIE_TOKENIZER_HPP_f28c53c53a48d38efafee7fb7004a01faaac9e22

#include <tdc_trie_mmap.hpp>
#include <utf8.h>
#include <fstream>
#include <vector>
#include <set>
#include <string>
#include <utility>
#include <cmath>
#include <cctype>
#include <cwctype>

namespace tdc {

template<typename String = u16string>
class trie_tokenizer {
private:
	typedef ::tdc::trie_mmap<String> trie_t;
	const trie_t *trie_;
	std::string line8;
	u16string line16;

	typedef std::pair<size_t, size_t> token_t;
	std::vector<token_t> tokens;
	typedef std::vector<size_t> output_t;
	output_t output;
	typedef std::vector<std::pair<size_t, output_t>> outputs_t;
	outputs_t outputs;

	std::pair<size_t, size_t> span;
	size_t weight;

	void _recurse_combos(size_t tmin, size_t tmax) {
		// If we can already see this would be a worse output than stored ones, don't even try...
		if (!outputs.empty() && outputs.back().first < weight - (span.second - tokens[tmin].first)) {
			return;
		}

		weight -= tokens[tmin].second - tokens[tmin].first;
		output.push_back(tmin);
		bool cut = true;
		for (size_t to = output.back() + 1; to < tmax; ++to) {
			// Recurse this combo
			if (tokens[to].first >= tokens[output.back()].second) {
				cut = false;
				_recurse_combos(to, tmax);
			}
		}
		if (cut) {
			// If this output is better than any previous seen output, wipe all stored outputs in favour of this one
			if (!outputs.empty() && weight < outputs.back().first) {
				outputs.clear();
			}
			// Only add an output if it is equal or better than currently stored outputs
			if (outputs.empty() || weight <= outputs.back().first) {
				outputs.push_back(std::make_pair(weight, output));
			}
		}
		output.pop_back();
		weight += tokens[tmin].second - tokens[tmin].first;
	}

	template<typename Printer>
	void _span_oneshot(Printer& pout, typename String::iterator begin, typename String::iterator end, bool garbage = false) {
		pout.span_open();
		pout.span_print(begin, end);
		pout.tokens_open();
		pout.token_print(begin, end, garbage);
		pout.tokens_close();
		pout.span_close();
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

		void token_print(typename String::iterator begin, typename String::iterator end, bool garbage = false) {
			(*out) << ' ';
			if (garbage) {
				(*out) << '*';
			}
			utf8::utf16to8(begin, end, std::ostream_iterator<char>(*out));
		}
	};

	static bool _compare_outputs(const outputs_t::value_type& a, const outputs_t::value_type& b) {
		if (a.first == b.first) {
			return a.second.size() < b.second.size();
		}
		return a.first < b.first;
	}

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
			while (!line8.empty() && tdc::isspace(line8[line8.size()-1])) {
				line8.resize(line8.size() - 1);
			}
			// Trim leading whitespace
			for (size_t i = 0; i < line8.size(); ++i) {
				if (!tdc::isspace(line8[i])) {
					line8.erase(line8.begin(), line8.begin() + i); // becomes a no-op for i=0
					break;
				}
			}
			// Don't even try to tokenize empty lines...
			if (line8.empty()) {
				continue;
			}

			// Convert UTF-8 to UTF-16
			line16.clear();
			utf8::utf8to16(line8.begin(), line8.end(), std::back_inserter(line16));

			// Now trim the UTF-16 version, just to catch the 1% crazy input that uses Unicode whitespace
			// Trim trailing whitespace
			while (!line16.empty() && std::iswspace(line16[line16.size()-1])) {
				line16.resize(line16.size() - 1);
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
			outputs.clear();

			// Find all possible valid tokens
			for (size_t co = 0; co < line16.size(); ++co) {
				typename trie_t::traverse_type trt(trie_t::npos, false);
				for (size_t ci = co; ci < line16.size(); ++ci) {
					trt = trie_->traverse(line16[ci], trt.first);
					if (trt.second == true) {
						tokens.push_back(std::make_pair(co, ci + 1));
					}
					else if (trt.first == trie_t::npos) {
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
				span = std::make_pair(tokens[tmin].first, tokens[tmax].second);
				for (size_t to = tmin; to < tokens.size(); ++to) {
					if (tokens[to].first < span.second) {
						span.second = std::max(span.second, tokens[to].second);
						tmax = to;
					}
					if (tokens[to].first >= span.second) {
						break;
					}
				}

				// Special case where there is invalid input before any found tokens, or between spans of tokens
				if (lastout < tokens[tmin].first) {
					_span_oneshot(pout, line16.begin() + lastout, line16.begin() + span.first, true);
				}
				lastout = span.second;

				// Build all possible combinations of tokens
				for (; tmin < tmax + 1; ++tmin) {
					weight = std::numeric_limits<size_t>::max();
					output.clear();
					_recurse_combos(tmin, tmax + 1);
				}

				// Sort outputs by weight and then by least number of tokens
				std::sort(outputs.begin(), outputs.end(), _compare_outputs);

				// If the first match is a single token perfect match, reduce the list to that one
				if (outputs.front().second.size() == 1 && outputs.front().first == std::numeric_limits<size_t>::max() - (span.second - span.first)) {
					outputs.resize(1);
				}

				// If there is only one best output, output it as separate spans instead
				if (outputs.size() == 1) {
					const output_t& output = outputs.front().second;
					size_t lastout = span.first;
					for (size_t j = 0; j < output.size(); ++j) {
						// Output any unclaimed characters between previous output and this token as an invalid token
						if (lastout < tokens[output[j]].first) {
							_span_oneshot(pout, line16.begin() + lastout, line16.begin() + tokens[output[j]].first, true);
						}
						_span_oneshot(pout, line16.begin() + tokens[output[j]].first, line16.begin() + tokens[output[j]].second);
						lastout = tokens[output[j]].second;
					}
					// Output any unclaimed characters between previous output and the end of the span as an invalid token
					if (lastout < span.second) {
						_span_oneshot(pout, line16.begin() + lastout, line16.begin() + span.second, true);
					}
					continue;
				}

				// Calculate how crazy we want the tokenization to get
				size_t max_tokens = outputs.front().second.size() + static_cast<size_t>(std::log(span.second - span.first) / std::log(2.0));

				// Actually output something...
				pout.span_open();
				pout.span_print(line16.begin() + span.first, line16.begin() + span.second);
				for (size_t i = 0; i < outputs.size(); ++i) {
					const output_t& output = outputs[i].second;
					// If we've gone beyond reasonable tokenization, bail out...
					if (output.size() > max_tokens) {
						break;
					}
					pout.tokens_open();
					size_t lastout = span.first;
					for (size_t j = 0; j < output.size(); ++j) {
						// Output any unclaimed characters between previous output and this token as an invalid token
						if (lastout < tokens[output[j]].first) {
							pout.token_print(line16.begin() + lastout, line16.begin() + tokens[output[j]].first, true);
						}
						pout.token_print(line16.begin() + tokens[output[j]].first, line16.begin() + tokens[output[j]].second);
						lastout = tokens[output[j]].second;
					}
					// Output any unclaimed characters between previous output and the end of the span as an invalid token
					if (lastout < span.second) {
						pout.token_print(line16.begin() + lastout, line16.begin() + span.second, true);
					}
					pout.tokens_close();
				}
				pout.span_close();
			}

			// Special case where there is invalid input after any found tokens
			if (lastout < line16.size()) {
				_span_oneshot(pout, line16.begin() + lastout, line16.end(), true);
			}
			pout.line_close();
		}
	}
};

}

#endif
