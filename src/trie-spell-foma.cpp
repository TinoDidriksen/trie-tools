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

#ifdef _MSC_VER
    #define _SECURE_SCL 0
    #define _CRT_SECURE_NO_DEPRECATE 1
    #define WIN32_LEAN_AND_MEAN
    #define VC_EXTRALEAN
    #define NOMINMAX
#endif

#include <tdc_trie.hpp>
#include <utf8.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <cctype>
#include <cwctype>

#ifdef _MSC_VER
	#include <windows.h>
#else
	#include <cstdio>
	#include <cstring>
	#include <popen_plus.h>
#endif

#define debugp
#define p

#ifdef _MSC_VER
HANDLE g_hChildStd_IN_Rd = 0;
HANDLE g_hChildStd_IN_Wr = 0;
HANDLE g_hChildStd_OUT_Rd = 0;
HANDLE g_hChildStd_OUT_Wr = 0;
#else
popen_plus_process *child = 0;
#endif

typedef std::unordered_map<tdc::u16string,bool> valid_words_t;
valid_words_t valid_words;

typedef tdc::trie<tdc::u16string> dict_t;
std::vector<dict_t> dicts(2);

struct word_t {
	size_t start, count;
	tdc::u16string u16buffer;
};
std::vector<word_t> words(8);
tdc::u16string u16buffer;
std::string cbuffer;

bool firstuse = true;
size_t cw = 0;

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

bool checkValidWord(const tdc::u16string& u16buffer) {
	debugp p("checkValidWord");
	p(u16buffer);

	cbuffer.clear();
	try {
		utf8::utf16to8(u16buffer.begin(), u16buffer.end(), std::back_inserter(cbuffer));
	}
	catch(...) {
		std::string msg("trie-tools-foma error: checkValidWord utf16:");
		msg.append(u16buffer.begin(), u16buffer.end());
		msg += " utf8:";
		msg += cbuffer;
		std::cerr << msg << std::endl;
	}
	cbuffer += '\n';

#ifdef _MSC_VER
	DWORD bytes = 0, bytes_read = 0;
	if (!WriteFile(g_hChildStd_IN_Wr, cbuffer.c_str(), (DWORD)cbuffer.length(), &bytes, 0) || bytes != cbuffer.length()) {
		showLastError(L"checkValidWord WriteFile");
		return false;
	}
	cbuffer.resize(4);
	if (!ReadFile(g_hChildStd_OUT_Rd, &cbuffer[0], 4, &bytes_read, 0) || bytes_read != 4) {
		showLastError(L"checkValidWord ReadFile 1");
		return false;
	}
	if (!PeekNamedPipe(g_hChildStd_OUT_Rd, 0, 0, 0, &bytes, 0)) {
		showLastError(L"checkValidWord PeekNamedPipe");
		return false;
	}
	if (bytes) {
		cbuffer.resize(4+bytes);
		if (!ReadFile(g_hChildStd_OUT_Rd, &cbuffer[4], bytes, &bytes_read, 0) || bytes != bytes_read) {
			showLastError(L"checkValidWord ReadFile 2");
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

void init(const std::string& fst, const std::string& dict=std::string()) {
	if (!firstuse) {
		return;
	}
	firstuse = false;

#ifdef _MSC_VER
	SECURITY_ATTRIBUTES saAttr = {sizeof(saAttr), 0, true};

	if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0)) {
		showLastError(L"SpellerInit CreatePipe 1");
		return;
	}
	if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) {
		showLastError(L"SpellerInit SetHandleInformation 1");
		return;
	}
	if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0)) {
		showLastError(L"SpellerInit CreatePipe 2");
		return;
	}
	if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0)) {
		showLastError(L"SpellerInit SetHandleInformation 2");
		return;
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
		return;
	}
	else {
		CloseHandle(piProcInfo.hProcess);
		CloseHandle(piProcInfo.hThread);
	}
#else
	std::string cmdline("flookup -b -x '");
	cmdline += fst;

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

	const char illu[] = "illu";
	if (!checkValidWord(tdc::u16string(illu, illu+sizeof(illu)-1))) {
		std::wstring msg = L"trie-tools-foma could not verify 'illu' as a correct word!\n";
		msg += L"Cmdline: ";
		msg.append(cmdline.begin(), cmdline.end());
		msg += '\n';
		std::cerr << std::string(msg.begin(), msg.end()) << std::endl;
		throw -1;
	}

	std::ifstream dictf(dict.c_str(), std::ios::binary);
	dicts[0].unserialize(dictf);
}

bool is_correct(const tdc::u16string& word) {
	size_t ichStart = 0, cchUse = word.size();
	const uint16_t *pwsz = word.c_str();

	// Always test the full given input
	words[0].u16buffer.resize(0);
	words[0].start = ichStart;
	words[0].count = cchUse;
	p(words[0].start);
	p(words[0].count);
	words[0].u16buffer.append(pwsz+ichStart, pwsz+ichStart+cchUse);
	cw = 1;
	p(words[cw-1].u16buffer);

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
		p(words[i].u16buffer);
		valid_words_t::iterator it = valid_words.find(words[i].u16buffer);

		if (it == valid_words.end()) {
			bool valid = checkValidWord(words[i].u16buffer);
			it = valid_words.insert(std::make_pair(words[i].u16buffer,valid)).first;

			if (!valid) {
				// If the word was not valid, fold it to lower case and try again
				u16buffer.resize(0);
				u16buffer.reserve(words[i].u16buffer.length());
				std::transform(words[i].u16buffer.begin(), words[i].u16buffer.end(), std::back_inserter(u16buffer), towlower);
				// Don't try again if the lower cased variant has already been tried
				valid_words_t::iterator itl = valid_words.find(u16buffer);
				if (itl != valid_words.end()) {
					it = itl;
				}
				else {
					valid = checkValidWord(u16buffer);
					it->second = valid; // Also mark the original mixed case variant as whatever the lower cased one was
					it = valid_words.insert(std::make_pair(u16buffer,valid)).first;
				}

				if (valid) {
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


/* CHLSpelling::FindAlternatives
*/
std::vector<tdc::u16string> find_alternatives(const tdc::u16string& word) {
	std::vector<tdc::u16string> alts;

	if (is_correct(word) != true) {
		dict_t::query_type qpDict, qpFly;
		if (words[cw-1].u16buffer.length() <= 3) {
			qpDict = dicts[0].query(words[cw-1].u16buffer, 1);
			qpFly = dicts[1].query(words[cw-1].u16buffer, 1);
		}
		else {
			qpDict = dicts[0].query(words[cw-1].u16buffer, 2);
			qpFly = dicts[1].query(words[cw-1].u16buffer, 2);
		}
		for (size_t i=1 ; i<3 ; ++i) {
			for (dict_t::query_type::iterator it=qpDict.begin() ; it != qpDict.end() ; ++it) {
				if (it->first == words[cw-1].u16buffer) {
					alts.clear();
					goto find_alternatives_end;
				}
				if (it->second == i) {
					u16buffer.clear();
					if (cw-1 != 0) {
						u16buffer.append(words[0].u16buffer.begin(), words[0].u16buffer.begin()+words[cw-1].start);
					}
					u16buffer.append(it->first);
					if (cw-1 != 0) {
						u16buffer.append(words[0].u16buffer.begin()+words[cw-1].start+words[cw-1].count, words[0].u16buffer.end());
					}
					if (std::find(alts.begin(), alts.end(), u16buffer) == alts.end()) {
						alts.push_back(u16buffer);
					}
				}
			}
			for (dict_t::query_type::iterator it=qpFly.begin() ; it != qpFly.end() ; ++it) {
				if (it->first == words[cw-1].u16buffer) {
					alts.clear();
					goto find_alternatives_end;
				}
				if (it->second == i) {
					u16buffer.clear();
					if (cw-1 != 0) {
						u16buffer.append(words[0].u16buffer.begin(), words[0].u16buffer.begin()+words[cw-1].start);
					}
					u16buffer.append(it->first);
					if (cw-1 != 0) {
						u16buffer.append(words[0].u16buffer.begin()+words[cw-1].start+words[cw-1].count, words[0].u16buffer.end());
					}
					if (std::find(alts.begin(), alts.end(), u16buffer) == alts.end()) {
						alts.push_back(u16buffer);
					}
				}
			}
		}
	}

find_alternatives_end:
	return alts;
}

int main(int argc, char *argv[]) {
	std::vector<std::string> args(argv, argv+argc);

	init(args[1], args[2]);

	std::cout << "@(#) International Ispell Version 3.1.20 (but really trie-tools-foma)" << std::endl;

	std::string line8;
	std::string out8;
	tdc::u16string line16;
	while (std::getline(std::cin, line8)) {
		while (!line8.empty() && std::isspace(line8[line8.size()-1])) {
			line8.resize(line8.size()-1);
		}
		if (line8.empty()) {
			continue;
		}

		size_t b = 0, e = 0;
		const char *spaces = " \t\r\n";
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
				std::cout << "*" << std::endl;
				continue;
			}

			const std::vector<tdc::u16string>& alts = find_alternatives(line16);
			if (alts.empty()) {
				std::cout << "# " << line8.substr(b, e) << " " << b << std::endl;
				continue;
			}

			std::cout << "& " << line8.substr(b, e) << " " << alts.size() << " " << b << ": ";
			for (size_t i=0 ; i<alts.size() ; ++i) {
				if (i != 0) {
					std::cout << ", ";
				}
				out8.clear();
				utf8::utf16to8(alts[i].begin(), alts[i].end(), std::back_inserter(out8));
				std::cout << out8;
			}
			std::cout << std::endl;

			if (e == std::string::npos) {
				break;
			}
		}
		std::cout << std::endl;
	}

	if (!firstuse) {
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
}
