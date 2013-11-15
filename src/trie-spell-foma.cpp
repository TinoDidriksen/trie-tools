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

#include <tdc_trie_speller.hpp>
#include <iostream>
#include <vector>
#include <string>

#ifdef _MSC_VER
	#include <windows.h>
#else
	#include <cstdio>
	#include <cstring>
	#include <popen_plus.h>
#endif

class trie_speller_foma : public tdc::trie_speller<> {
protected:
	#ifdef _MSC_VER
	HANDLE g_hChildStd_IN_Rd;
	HANDLE g_hChildStd_IN_Wr;
	HANDLE g_hChildStd_OUT_Rd;
	HANDLE g_hChildStd_OUT_Wr;
	#else
	popen_plus_process *child;
	#endif

	void showLastError(const std::wstring& err) {
		std::wstring msg = L"trie-tools-foma error location: ";
		msg += err;
		msg += L"\n\n";
	#ifdef _MSC_VER
		wchar_t *fmt = 0;
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_ALLOCATE_BUFFER, 0, GetLastError(), MAKELANGID(LANG_NEUTRAL,SUBLANG_NEUTRAL), (LPWSTR)&fmt, 0, 0);
		msg += L"GetLastError: ";
		msg += fmt;
		msg += '\n';
		LocalFree(fmt);
	#else
		msg += L"strerror: ";
		std::string tmp = strerror(errno);
		msg += std::wstring(tmp.begin(), tmp.end());
	#endif
		std::cerr << std::string(msg.begin(), msg.end()) << std::endl;
	}

	bool is_correct_word(const tdc::u16string& u16buffer) {
		cbuffer.clear();
		try {
			utf8::utf16to8(u16buffer.begin(), u16buffer.end(), std::back_inserter(cbuffer));
		}
		catch(...) {
			std::string msg("trie-tools-foma error: is_correct_word utf16:");
			msg.append(u16buffer.begin(), u16buffer.end());
			msg += " utf8:";
			msg += cbuffer;
			std::cerr << msg << std::endl;
		}
		cbuffer += '\n';

	#ifdef _MSC_VER
		DWORD bytes = 0, bytes_read = 0;
		if (!WriteFile(g_hChildStd_IN_Wr, cbuffer.c_str(), (DWORD)cbuffer.length(), &bytes, 0) || bytes != cbuffer.length()) {
			showLastError(L"is_correct_word WriteFile");
			return false;
		}
		cbuffer.resize(4);
		if (!ReadFile(g_hChildStd_OUT_Rd, &cbuffer[0], 4, &bytes_read, 0) || bytes_read != 4) {
			showLastError(L"is_correct_word ReadFile 1");
			return false;
		}
		if (!PeekNamedPipe(g_hChildStd_OUT_Rd, 0, 0, 0, &bytes, 0)) {
			showLastError(L"is_correct_word PeekNamedPipe");
			return false;
		}
		if (bytes) {
			cbuffer.resize(4+bytes);
			if (!ReadFile(g_hChildStd_OUT_Rd, &cbuffer[4], bytes, &bytes_read, 0) || bytes != bytes_read) {
				showLastError(L"is_correct_word ReadFile 2");
				return false;
			}
		}
	#else
		fwrite(cbuffer.c_str(), sizeof(cbuffer[0]), cbuffer.length(), child->write_fp);
		fflush(child->write_fp);
		cbuffer.clear();
		char tmp[4096];
		do {
			fgets(tmp, sizeof(tmp), child->read_fp);
			cbuffer += tmp;
		} while (cbuffer[cbuffer.length()-1] != '\n' || cbuffer[cbuffer.length()-2] != '\n');
	#endif
		return cbuffer[0] != '+' || cbuffer[1] != '?' || cbuffer[2] != '\n';
	}

public:
	trie_speller_foma(const std::string& fst, const std::string& dict) :
	tdc::trie_speller<>(dict),
	#ifdef _MSC_VER
	g_hChildStd_IN_Rd(0),
	g_hChildStd_IN_Wr(0),
	g_hChildStd_OUT_Rd(0),
	g_hChildStd_OUT_Wr(0)
	#else
	child(0)
	#endif
	{
	#ifdef _MSC_VER
		SECURITY_ATTRIBUTES saAttr = {sizeof(saAttr), 0, true};

		if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0)) {
			showLastError(L"SpellerInit CreatePipe 1");
			throw -1;
		}
		if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) {
			showLastError(L"SpellerInit SetHandleInformation 1");
			throw -1;
		}
		if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0)) {
			showLastError(L"SpellerInit CreatePipe 2");
			throw -1;
		}
		if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0)) {
			showLastError(L"SpellerInit SetHandleInformation 2");
			throw -1;
		}

		PROCESS_INFORMATION piProcInfo = {0};
		STARTUPINFO siStartInfo = {sizeof(siStartInfo)};

		siStartInfo.hStdError = g_hChildStd_OUT_Wr;
		siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
		siStartInfo.hStdInput = g_hChildStd_IN_Rd;
		siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

		std::wstring cmdline(L"flookup -b -x '");
		utf8::utf8to16(fst.begin(), fst.end(), std::back_inserter(cmdline));
		cmdline += '\'';
		cmdline.append(1, 0);

		BOOL bSuccess = CreateProcess(0,
			&cmdline[0],
			0,
			0,
			TRUE,
			CREATE_NO_WINDOW | BELOW_NORMAL_PRIORITY_CLASS,
			0,
			0,
			&siStartInfo,
			&piProcInfo);

		if (!bSuccess) {
			std::wstring msg = L"trie-tools-foma could not start flookup.exe!\n\n";
			msg += L"Cmdline: ";
			msg += cmdline.c_str();
			msg += '\n';
			wchar_t *fmt = 0;
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_ALLOCATE_BUFFER, 0, GetLastError(), MAKELANGID(LANG_NEUTRAL,SUBLANG_NEUTRAL), (LPWSTR)&fmt, 0, 0);
			msg += L"GetLastError: ";
			msg += fmt;
			msg += '\n';
			LocalFree(fmt);
			std::cerr << std::string(msg.begin(), msg.end()) << std::endl;
			throw -1;
		}
		else {
			CloseHandle(piProcInfo.hProcess);
			CloseHandle(piProcInfo.hThread);
		}
	#else
		std::string cmdline("flookup -b -x '");
		cmdline += fst;
		cmdline += '\'';

		child = popen_plus(cmdline.c_str());
		if (child == 0) {
			std::string msg = "trie-tools-foma could not start flookup!\n";
			msg += "Cmdline: ";
			msg += cmdline.c_str();
			msg += '\n';
			msg += "strerror: ";
			msg += strerror(errno);
			std::cerr << msg << std::endl;
			throw -1;
		}
	#endif

		bool working = false;
		size_t i = 0;
		for (dict_t::const_iterator it = dicts[0].begin() ; it != dicts[0].end() ; ++it) {
			if (is_correct_word(*it)) {
				working = true;
				break;
			}
			if (++i > 10) {
				break;
			}
		}
		if (!working) {
			std::string msg = "trie-tools-foma could not verify any of the 10 first words as being correct!\n";
			msg += "Cmdline: ";
			msg.append(cmdline.begin(), cmdline.end());
			msg += '\n';
			std::cerr << msg << std::endl;
			throw -1;
		}
	}

	~trie_speller_foma() {
	#ifdef _MSC_VER
		CloseHandle(g_hChildStd_IN_Rd);
		CloseHandle(g_hChildStd_IN_Wr);
		CloseHandle(g_hChildStd_OUT_Rd);
		CloseHandle(g_hChildStd_OUT_Wr);
	#else
		popen_plus_kill(child);
		popen_plus_close(child);
	#endif
	}
};

int main(int argc, char *argv[]) {
	std::vector<std::string> args(argv, argv+argc);

	trie_speller_foma speller(args[1], args[2]);

	speller.check_stream_utf8(std::cin, std::cout);
}
