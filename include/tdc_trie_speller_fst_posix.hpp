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
#ifndef TRIE_SPELL_FST_POSIX_HPP_f28c53c53a48d38efafee7fb7004a01faaac9e22
#define TRIE_SPELL_FST_POSIX_HPP_f28c53c53a48d38efafee7fb7004a01faaac9e22

#include <iostream>
#include <vector>
#include <string>
#include <exception>
#include <cstdio>
#include <cstring>
#include <popen_plus.h>

namespace tdc {

class trie_speller_fst_helper {
private:
	popen_plus_process *child;

public:
	std::string formatLastError(std::string msg = "") {
		if (!msg.empty()) {
			msg += ' ';
		}
		msg += "strerror: ";
		msg += strerror(errno);
	}

	void is_correct_word(std::string& cbuffer) {
		fwrite(cbuffer.c_str(), sizeof(cbuffer[0]), cbuffer.length(), child->write_fp);
		fflush(child->write_fp);
		cbuffer.clear();
		char tmp[4096];
		do {
			fgets(tmp, sizeof(tmp), child->read_fp);
			cbuffer += tmp;
		} while (cbuffer[cbuffer.length()-1] != '\n' || cbuffer[cbuffer.length()-2] != '\n');
	}

	trie_speller_fst_helper(const std::string& bin, const std::string& fst) :
	child(0)
	{
		std::string cmdline(bin.size() + fst.size() + 1, 0);
		cmdline.resize(sprintf(&cmdline[0], bin.c_str(), fst.c_str()));

		child = popen_plus(cmdline.c_str());
		if (child == 0) {
			std::string msg = "trie-tools-foma could not start flookup!\nCmdline: ";
			msg += cmdline.c_str();
			msg += '\n';
			msg = formatLastError(msg);
			throw std::runtime_error(msg.c_str());
		}
	}

	~trie_speller_fst_helper() {
		popen_plus_kill(child);
		popen_plus_close(child);
	}
};

}

#endif
