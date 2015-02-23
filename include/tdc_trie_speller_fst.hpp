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
#ifndef TRIE_SPELL_FST_HPP_f28c53c53a48d38efafee7fb7004a01faaac9e22
#define TRIE_SPELL_FST_HPP_f28c53c53a48d38efafee7fb7004a01faaac9e22

#ifdef _WIN32
	#include <tdc_trie_speller_fst_windows.hpp>
#else
	#include <tdc_trie_speller_fst_posix.hpp>
#endif

#include <tdc_trie_speller.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>

namespace tdc {

class trie_speller_fst : public tdc::trie_speller<> {
protected:
	trie_speller_fst_helper helper;

	void showLastError(const std::string& err) {
		std::string msg = "trie-tools-fst error location: ";
		msg += err;
		msg += "\n\n";
		msg = helper.formatLastError(msg);
		std::cerr << std::string(msg.begin(), msg.end()) << std::endl;
	}

	bool is_correct_word(const tdc::u16string& u16buffer) {
		cbuffer.clear();
		try {
			utf8::utf16to8(u16buffer.begin(), u16buffer.end(), std::back_inserter(cbuffer));
		}
		catch(...) {
			std::string msg("trie-tools-fst error: is_correct_word utf16:");
			msg.append(u16buffer.begin(), u16buffer.end());
			msg += " utf8:";
			msg += cbuffer;
			std::cerr << msg << std::endl;
		}
		cbuffer += '\n';

		helper.is_correct_word(cbuffer);

		return cbuffer[0] != '+' || cbuffer[1] != '?' || cbuffer[2] != '\n';
	}

public:
	trie_speller_fst(const std::string& bin, const std::string& fst, const std::string& dict) :
	tdc::trie_speller<>(dict),
	helper(bin, fst)
	{
		bool working = false;
		size_t i = 0;
		for (trie_mmap_t::const_iterator it = trie.begin(); it != trie.end(); ++it) {
			if (is_correct_word(*it)) {
				working = true;
				break;
			}
			if (++i > 10) {
				break;
			}
		}
		if (!working) {
			throw std::runtime_error("trie-tools-fst could not verify any of the 10 first words as being correct!");
		}
	}
};

}

#endif
