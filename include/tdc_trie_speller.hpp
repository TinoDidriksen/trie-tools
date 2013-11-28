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
#ifndef TDC_TRIE_SPELLER_HPP_f28c53c53a48d38efafee7fb7004a01faaac9e22
#define TDC_TRIE_SPELLER_HPP_f28c53c53a48d38efafee7fb7004a01faaac9e22

#include <tdc_trie.hpp>
#include <utf8.h>
#include <fstream>
#include <unordered_map>
#include <cctype>
#include <cwctype>
#include <cmath>

namespace tdc {

template<typename String=u16string>
class trie_speller {
public:
	trie_speller(const std::string& dict) :
	dicts(2),
	words(8),
	cw(0)
	{
		std::ifstream dictf(dict.c_str(), std::ios::binary);
		if (!dictf) {
			std::cerr << "Could not open trie " << dict << " for reading!" << std::endl;
			throw -1;
		}
		dicts[0].unserialize(dictf);
	}

	virtual ~trie_speller() {
	}

	virtual bool is_correct_word(const String& word) {
		return (dicts[0].find(word) != dicts[0].end());
	}

	virtual bool is_correct(const String& word) {
		size_t ichStart = 0, cchUse = word.size();
		const uint16_t *pwsz = word.c_str();

		// Always test the full given input
		words[0].u16buffer.resize(0);
		words[0].start = ichStart;
		words[0].count = cchUse;
		words[0].u16buffer.append(pwsz+ichStart, pwsz+ichStart+cchUse);
		cw = 1;

		if (cchUse > 1) {
			size_t count = cchUse;
			while (count && (std::iswpunct(pwsz[ichStart+count-1]) || std::iswspace(pwsz[ichStart+count-1]))) {
				--count;
			}
			if (count != cchUse) {
				// If the input ended with punctuation, test input with punctuation trimmed from the end
				words[cw].u16buffer.resize(0);
				words[cw].start = ichStart;
				words[cw].count = count;
				words[cw].u16buffer.append(pwsz+words[cw].start, pwsz+words[cw].start+words[cw].count);
				++cw;
			}

			size_t start = ichStart, count2 = cchUse;
			while (start < ichStart+cchUse && (std::iswpunct(pwsz[start]) || std::iswspace(pwsz[start]))) {
				++start;
				--count2;
			}
			if (start != ichStart) {
				// If the input started with punctuation, test input with punctuation trimmed from the start
				words[cw].u16buffer.resize(0);
				words[cw].start = start;
				words[cw].count = count2;
				words[cw].u16buffer.append(pwsz+words[cw].start, pwsz+words[cw].start+words[cw].count);
				++cw;
			}

			if (start != ichStart && count != cchUse) {
				// If the input both started and ended with punctuation, test input with punctuation trimmed from both sides
				words[cw].u16buffer.resize(0);
				words[cw].start = start;
				words[cw].count = count - (cchUse - count2);
				words[cw].u16buffer.append(pwsz+words[cw].start, pwsz+words[cw].start+words[cw].count);
				++cw;
			}
		}

		for (size_t i=0 ; i<cw ; ++i) {
			typename valid_words_t::iterator it = valid_words.find(words[i].u16buffer);

			if (it == valid_words.end()) {
				bool valid = is_correct_word(words[i].u16buffer);
				it = valid_words.insert(std::make_pair(words[i].u16buffer,valid)).first;

				if (!valid) {
					// If the word was not valid, fold it to lower case and try again
					u16buffer.resize(0);
					u16buffer.reserve(words[i].u16buffer.size());
					std::transform(words[i].u16buffer.begin(), words[i].u16buffer.end(), std::back_inserter(u16buffer), towlower);
					// Don't try again if the lower cased variant has already been tried
					typename valid_words_t::iterator itl = valid_words.find(u16buffer);
					if (itl != valid_words.end()) {
						it->second = itl->second;
						it = itl;
					}
					else {
						valid = is_correct_word(u16buffer);
						it->second = valid; // Also mark the original mixed case variant as whatever the lower cased one was
						it = valid_words.insert(std::make_pair(u16buffer,valid)).first;
					}

					if (it->second) {
						dicts[1].add(words[i].u16buffer);
						dicts[1].add(u16buffer);
					}
				}
				else {
					dicts[1].add(words[i].u16buffer);
				}
			}

			if (it->second == true) {
				return true;
			}
		}

		return false;
	}

	virtual std::vector<String> find_alternatives(const String& word) {
		std::vector<String> alts;

		if (is_correct(word) != true) {
			size_t dist = std::max(static_cast<size_t>(1), static_cast<size_t>(std::log(words[cw - 1].u16buffer.size()) / std::log(2)));
			typename dict_t::query_type qps[] = { dicts[0].query(words[cw - 1].u16buffer, dist), dicts[1].query(words[cw - 1].u16buffer, dist) };
			for (size_t i=1 ; i<3 ; ++i) {
				for (size_t qi = 0; qi < 2; ++qi) {
					for (typename dict_t::query_type::iterator it = qps[qi].begin(); it != qps[qi].end(); ++it) {
						if (it->first == words[cw - 1].u16buffer) {
							alts.clear();
							goto find_alternatives_end;
						}
						if (it->second == i) {
							u16buffer.clear();
							if (cw - 1 != 0) {
								u16buffer.append(words[0].u16buffer.begin(), words[0].u16buffer.begin() + words[cw - 1].start);
							}
							u16buffer.append(it->first);
							if (cw - 1 != 0) {
								u16buffer.append(words[0].u16buffer.begin() + words[cw - 1].start + words[cw - 1].count, words[0].u16buffer.end());
							}
							if (std::find(alts.begin(), alts.end(), u16buffer) == alts.end()) {
								alts.push_back(u16buffer);
							}
						}
					}
				}
			}
		}

	find_alternatives_end:
		return alts;
	}

	virtual void check_stream_utf8(std::istream& in, std::ostream& out) {
		out << "@(#) International Ispell Version 3.1.20 (but really trie-tools " << TRIE_VERSION_MAJOR << "." << TRIE_VERSION_MINOR << "." << TRIE_VERSION_PATCH << "." << TRIE_REVISION << ")" << std::endl;

		std::string line8;
		std::string out8;
		tdc::u16string line16;
		while (std::getline(in, line8)) {
			while (!line8.empty() && std::isspace(line8[line8.size()-1])) {
				line8.resize(line8.size()-1);
			}
			if (line8.empty()) {
				continue;
			}

			size_t b = 0, e = 0;
			const char *spaces = " \t\r\n!:;?{}\"";
			for (;;) {
				b = line8.find_first_not_of(spaces, e);
				e = line8.find_first_of(spaces, b);
				if (b == std::string::npos) {
					break;
				}

				line16.clear();
				if (e == std::string::npos) {
					utf8::utf8to16(line8.begin()+b, line8.end(), std::back_inserter(line16));
				}
				else {
					utf8::utf8to16(line8.begin()+b, line8.begin()+e, std::back_inserter(line16));
				}

				if (is_correct(line16)) {
					out << "*" << std::endl;
					continue;
				}

				const std::vector<tdc::u16string>& alts = find_alternatives(line16);
				if (alts.empty()) {
					out << "# " << line8.substr(b, e-b) << " " << b << std::endl;
					continue;
				}

				out << "& " << line8.substr(b, e-b) << " " << alts.size() << " " << b << ": ";
				for (size_t i=0 ; i<alts.size() ; ++i) {
					if (i != 0) {
						out << ", ";
					}
					out8.clear();
					utf8::utf16to8(alts[i].begin(), alts[i].end(), std::back_inserter(out8));
					out << out8;
				}
				out << std::endl;

				if (e == std::string::npos) {
					break;
				}
			}
			out << std::endl;
		}
	}

protected:
	typedef tdc::trie<String> dict_t;
	std::vector<dict_t> dicts;

	template<typename T>
	struct hash_any_string {
		size_t operator()(const T& str) const {
			std::wstring wstr(str.begin(), str.end());
			return std::hash<std::wstring>()(wstr);
		}
	};

	typedef std::unordered_map<String,bool,hash_any_string<String>> valid_words_t;
	valid_words_t valid_words;

	struct word_t {
		size_t start, count;
		String u16buffer;
	};
	std::vector<word_t> words;
	String u16buffer;
	std::string cbuffer;

	size_t cw;
};

}

#endif
