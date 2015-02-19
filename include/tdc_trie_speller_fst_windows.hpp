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
#ifndef TRIE_SPELL_FST_WINDOWS_HPP_f28c53c53a48d38efafee7fb7004a01faaac9e22
#define TRIE_SPELL_FST_WINDOWS_HPP_f28c53c53a48d38efafee7fb7004a01faaac9e22

#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <windows.h>

namespace tdc {

class trie_speller_fst_helper {
private:
	HANDLE g_hChildStd_IN_Rd;
	HANDLE g_hChildStd_IN_Wr;
	HANDLE g_hChildStd_OUT_Rd;
	HANDLE g_hChildStd_OUT_Wr;

public:
	std::string formatLastError(std::string msg = "") {
		if (!msg.empty()) {
			msg += ' ';
		}
		char *fmt = 0;
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, 0, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), (LPSTR)&fmt, 0, 0);
		msg += "GetLastError: ";
		msg += fmt;
		msg += '\n';
		LocalFree(fmt);
		return msg;
	}

	void is_correct_word(std::string& cbuffer) {
		DWORD bytes = 0, bytes_read = 0;
		if (!WriteFile(g_hChildStd_IN_Wr, cbuffer.c_str(), (DWORD)cbuffer.length(), &bytes, 0) || bytes != cbuffer.length()) {
			std::string msg = formatLastError("is_correct_word WriteFile");
			throw std::runtime_error(msg.c_str());
		}
		cbuffer.resize(4);
		if (!ReadFile(g_hChildStd_OUT_Rd, &cbuffer[0], 4, &bytes_read, 0) || bytes_read != 4) {
			std::string msg = formatLastError("is_correct_word ReadFile 1");
			throw std::runtime_error(msg.c_str());
		}
		if (!PeekNamedPipe(g_hChildStd_OUT_Rd, 0, 0, 0, &bytes, 0)) {
			std::string msg = formatLastError("is_correct_word PeekNamedPipe");
			throw std::runtime_error(msg.c_str());
		}
		if (bytes) {
			cbuffer.resize(4+bytes);
			if (!ReadFile(g_hChildStd_OUT_Rd, &cbuffer[4], bytes, &bytes_read, 0) || bytes != bytes_read) {
				std::string msg = formatLastError("is_correct_word ReadFile 2");
				throw std::runtime_error(msg.c_str());
			}
		}
	}

	trie_speller_fst_helper(const std::string& bin, const std::string& fst) :
	g_hChildStd_IN_Rd(0),
	g_hChildStd_IN_Wr(0),
	g_hChildStd_OUT_Rd(0),
	g_hChildStd_OUT_Wr(0)
	{
		SECURITY_ATTRIBUTES saAttr = {sizeof(saAttr), 0, true};

		if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0)) {
			std::string msg = formatLastError("SpellerInit CreatePipe 1");
			throw std::runtime_error(msg.c_str());
		}
		if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) {
			std::string msg = formatLastError("SpellerInit SetHandleInformation 1");
			throw std::runtime_error(msg.c_str());
		}
		if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0)) {
			std::string msg = formatLastError("SpellerInit CreatePipe 2");
			throw std::runtime_error(msg.c_str());
		}
		if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0)) {
			std::string msg = formatLastError("SpellerInit SetHandleInformation 2");
			throw std::runtime_error(msg.c_str());
		}

		PROCESS_INFORMATION piProcInfo = {0};
		STARTUPINFOA siStartInfo = {sizeof(siStartInfo)};

		siStartInfo.hStdError = g_hChildStd_OUT_Wr;
		siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
		siStartInfo.hStdInput = g_hChildStd_IN_Rd;
		siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

		std::string cmdline(bin.size() + fst.size() + 1, 0);
		cmdline.resize(sprintf(&cmdline[0], bin.c_str(), fst.c_str()));

		BOOL bSuccess = CreateProcessA(0,
			const_cast<char*>(cmdline.c_str()),
			0,
			0,
			TRUE,
			CREATE_NO_WINDOW | BELOW_NORMAL_PRIORITY_CLASS,
			0,
			0,
			&siStartInfo,
			&piProcInfo);

		if (!bSuccess) {
			std::string msg("trie-tools-foma could not start flookup.exe!\nCmdline: ");
			msg += cmdline.c_str();
			msg += '\n';
			msg = formatLastError(msg);
			throw std::runtime_error(msg.c_str());
		}

		CloseHandle(piProcInfo.hProcess);
		CloseHandle(piProcInfo.hThread);
	}

	~trie_speller_fst_helper() {
		CloseHandle(g_hChildStd_IN_Rd);
		CloseHandle(g_hChildStd_IN_Wr);
		CloseHandle(g_hChildStd_OUT_Rd);
		CloseHandle(g_hChildStd_OUT_Wr);
	}
};

}

#endif
