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
#ifndef TDC_TRIE_MMAP_HPP_f28c53c53a48d38efafee7fb7004a01faaac9e22
#define TDC_TRIE_MMAP_HPP_f28c53c53a48d38efafee7fb7004a01faaac9e22

#define BOOST_DATE_TIME_NO_LIB 1

#include <tdc_trie.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <cstdio>
#include <stdint.h>
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace tdc {

namespace bi = ::boost::interprocess;

template<typename NS, typename T, typename Y>
inline T findchild(const char *p, const NS& nodes, const T& t, size_t n, const Y& y) {
	T it, first = t;
	size_t count = n, step;

	while (count > 0) {
		it = first;
		step = count / 2;
		it += step;
		if (nodes[*it].self(p) < y) {
			first = ++it;
			count -= step + 1;
		}
		else {
			count = step;
		}
	}
	if (first != t + n && nodes[*first].self(p) != y) {
		first = t + n;
	}
	return first;
}

template<typename String=u16string, typename Count=uint32_t>
class trie_mmap {
private:

	class trie_node {
	public:
		typedef trie_node node_type;
		typedef const Count* children_type;
		typedef std::vector<const node_type*> query_path_type;
		typedef std::map<String,size_t> query_type;
		typedef trie_mmap root_type;

		Count n;

		bool terminal(const char *p) const {
			return *reinterpret_cast<const uint16_t*>(p + n + sizeof(uint16_t)) != 0;
		}

		typename String::value_type self(const char *p) const {
			return *reinterpret_cast<const uint16_t*>(p + n);
		}

		Count num_terminals(const char *p) const {
			return *reinterpret_cast<const Count*>(p + n + sizeof(uint16_t) + sizeof(uint16_t));
		}

		children_type children(const char *p) const {
			return reinterpret_cast<children_type>(p + n + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(Count) + sizeof(Count));
		}

		Count num_children(const char *p) const {
			return *reinterpret_cast<const Count*>(p + n + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(Count));
		}

		void buildString(const char *p, const query_path_type& qp, String& in) const {
			in.reserve(qp.size());
			for (typename query_path_type::const_iterator it = qp.begin() ; it != qp.end() ; ++it) {
				in.push_back((*it)->self(p));
			}
		}

	public:

		void query(const root_type& root, const String& entry, size_t pos, query_type& collected, query_path_type& qp, size_t maxdist=0, size_t curdist=0) const {
			qp.push_back(this);

			const char *p = const_char_p(root.mreg.get_address());
			auto cs = children(p);
			auto cn = num_children(p);

			if (pos < entry.size()) {
				children_type child = findchild(p, root.nodes, cs, cn, entry[pos]);
				if (child != cs + cn) {
					root.nodes[*child].query(root, entry, pos+1, collected, qp, maxdist, curdist);
				}
			}

			if (curdist < maxdist) {
				for (children_type child = cs ; child != cs + cn ; ++child) {
					if (pos >= entry.size() || root.nodes[*child].self(p) != entry[pos]) {
						root.nodes[*child].query(root, entry, pos, collected, qp, maxdist, curdist+1);
						root.nodes[*child].query(root, entry, pos+1, collected, qp, maxdist, curdist+1);
					}
					for (size_t i = 1 ; pos+i < entry.size() ; ++i) {
						if (root.nodes[*child].self(p) == entry[pos + i]) {
							root.nodes[*child].query(root, entry, pos+i+1, collected, qp, maxdist, curdist+i);
						}
					}
				}
			}

			if (terminal(p)) {
				size_t dist = curdist;
				if (pos < entry.size()) {
					dist += entry.size() - pos;
				}
				else {
					dist += pos - entry.size();
				}
				if (dist <= maxdist) {
					String out;
					buildString(p, qp, out);
					typename query_type::iterator ins = collected.insert(std::make_pair(out, dist)).first;
					ins->second = std::min(ins->second, dist);
				}
			}

			qp.pop_back();
		}
	};

	friend class trie_node;

	typedef trie_node node_type;
	typedef std::vector<const node_type*> query_path_type;

	const node_type *nodes;
	Count num_nodes;
	bi::file_mapping fmap;
	bi::mapped_region mreg;

public:
	class const_iterator {
	private:
		friend class trie_mmap;
		const trie_mmap *owner;
		std::vector<Count> path;

	public:
		const_iterator(const trie_mmap *owner = 0) :
		owner(owner)
		{
		}

		const_iterator(const trie_mmap *owner, Count n) :
		owner(owner),
		path(1, n)
		{
			const char *p = const_char_p(owner->mreg.get_address());
			if (owner && n < owner->size() && !owner->nodes[n].terminal(p)) {
				while (owner->nodes[n].num_children(p)) {
					n = *owner->nodes[n].children(p);
					path.push_back(n);
				}
			}
			if (owner && n == owner->size()) {
				path.clear();
			}
		}

		String operator*() const {
			String rv;
			rv.reserve(path.size());
			const char *p = const_char_p(owner->mreg.get_address());
			for (size_t i = 1; i<path.size(); ++i) {
				rv += owner->nodes[path[i]].self(p);
			}
			return rv;
		}

		bool operator==(const const_iterator& o) const {
			return owner == o.owner && path == o.path;
		}

		bool operator!=(const const_iterator& o) const {
			return !(*this == o);
		}

		const_iterator& operator++() {
			while (!path.empty()) {
				Count old = path.back();
				path.pop_back();

				if (path.empty()) {
					break;
				}

				const char *p = const_char_p(owner->mreg.get_address());
				auto cs = owner->nodes[path.back()].children(p);
				auto cn = owner->nodes[path.back()].num_children(p);

				typename trie_node::children_type child = findchild(p, owner->nodes, cs, cn, owner->nodes[old].self(p));
				++child;
				if (child != cs + cn) {
					Count n = *child;
					path.push_back(n);
					while (owner->nodes[n].num_children(p)) {
						n = *owner->nodes[n].children(p);
						path.push_back(n);
					}
					goto plus_return;
				}
				if (owner->nodes[path.back()].terminal(p)) {
					break;
				}
			}
		plus_return:
			return *this;
		}
	};

	class browser {
	private:
		const trie_mmap *owner;
		Count node;

	public:
		class browser_out {
		private:
			const trie_mmap *owner;
			Count node;

		public:
			browser_out(const trie_mmap *owner = 0, Count node = npos) :
				owner(owner),
				node(node) {
			}

			const_iterator begin() const {
				return const_iterator(owner, node);
			}

			const_iterator end() const {
				return const_iterator(owner);
			}
		};

		class browser_iter {
		private:
			const trie_mmap *owner;
			Count node;
			Count which;

		public:
			browser_iter(const trie_mmap *owner = 0, Count node = npos, Count which = 0) :
				owner(owner),
				node(node),
				which(which) {
			}

			browser_out values() const {
				const char *p = const_char_p(owner->mreg.get_address());
				return browser_out(owner, *(owner->nodes[node].children(p) + which));
			}

			std::pair<typename String::value_type, Count> operator*() const {
				const char *p = const_char_p(owner->mreg.get_address());
				const typename trie_node::children_type& children = owner->nodes[node].children(p);
				return std::make_pair(owner->nodes[children[which]].self(p), owner->nodes[children[which]].num_terminals(p));
			}

			bool operator==(const browser_iter& o) {
				return (owner == o.owner) && (node == o.node) && (which == o.which);
			}

			bool operator!=(const browser_iter& o) {
				return !(*this == o);
			}

			browser_iter& operator++() {
				++which;
				return *this;
			}
		};

		browser(const trie_mmap *owner = 0, Count node = npos) :
			owner(owner),
			node(node) {
		}

		browser_iter begin() const {
			return browser_iter(owner, node, 0);
		}

		browser_iter end() const {
			const char *p = const_char_p(owner->mreg.get_address());
			return browser_iter(owner, node, owner->nodes[node].num_children(p));
		}
	};

	friend class const_iterator;
	friend class browser;

	typedef std::map<String,size_t> query_type;
	typedef std::pair<size_t,bool> traverse_type;
	typedef String value_type;
	enum {
		npos = static_cast<Count>(0)
	};

	trie_mmap(const char *fname) :
		fmap(fname, bi::read_only),
		mreg(fmap, bi::read_only)
	{
		const char *reg = static_cast<const char*>(mreg.get_address());
		if (memcmp(reg, "TRIE", 4)) {
			throw std::runtime_error("Unserialize stream did not start with magic byte sequence TRIE");
		}
		reg += 4;

		uint32_t rev = 0;
		memcpy(&rev, reg, sizeof(rev));
		if (rev != TRIE_SERIALIZED_REVISION) {
			char _msg[] = "Unserialize expected revision %u but data had revision %u";
			std::string msg(sizeof(_msg) + 11 + 11 + 1, 0);
			msg.resize(sprintf(&msg[0], _msg, TRIE_SERIALIZED_REVISION, rev));
			throw std::runtime_error(msg);
		}
		reg += sizeof(rev);

		uint16_t s;
		memcpy(&s, reg, sizeof(s));
		if (s != sizeof(typename String::value_type)) {
			char _msg[] = "Unserialize expected code unit width %u but data had width %u";
			std::string msg(sizeof(_msg) + 11 + 11 + 1, 0);
			msg.resize(sprintf(&msg[0], _msg, sizeof(typename String::value_type), s));
			throw std::runtime_error(msg);
		}
		reg += sizeof(s);
		reg += sizeof(s); // Compressed flag

		memcpy(&num_nodes, reg, sizeof(num_nodes));
		reg += sizeof(num_nodes);
		for (size_t n = 0; n < num_nodes; ++n) {
			reg += sizeof(s); // self
			reg += sizeof(s); // terminal
			reg += sizeof(Count); // num_terminals

			Count c;
			memcpy(&c, reg, sizeof(c));
			reg += sizeof(c);
			reg += c*sizeof(Count); // children
		}
		nodes = reinterpret_cast<const node_type*>(reg);
	}

	size_t size() const {
		return num_nodes;
	}

	const_iterator begin() const {
		return const_iterator(this, 0);
	}

	const_iterator end() const {
		return const_iterator(this);
	}

	query_type query(const String& entry, size_t maxdist = 0) const {
		query_type matches;
		if (!entry.empty()) {
			query_path_type qp;
			qp.reserve(entry.size()+maxdist+2);
			nodes[0].query(*this, entry, 0, matches, qp, maxdist);
		}
		return matches;
	}

	const_iterator find(const String& entry) const {
		const_iterator rv = end();
		const char *p = const_char_p(mreg.get_address());
		auto cs = nodes[0].children(p);
		auto cn = nodes[0].num_children(p);
		typename node_type::children_type child = findchild(p, nodes, cs, cn, entry[0]);
		if (child != cs + cn) {
			rv.path.clear();
			rv.path.push_back(0);
			rv.path.push_back(*child);
			for (size_t i=1 ; i<entry.size() ; ++i) {
				Count second = *child;
				auto cs = nodes[second].children(p);
				auto cn = nodes[second].num_children(p);
				child = findchild(p, nodes, cs, cn, entry[i]);
				if (child == cs + cn) {
					rv = end();
					break;
				}
				rv.path.push_back(*child);
			}
			if (!rv.path.empty() && nodes[rv.path.back()].terminal(p) == false) {
				rv = end();
			}
		}
		return rv;
	}

	traverse_type traverse(typename String::value_type c, size_t n=npos) const {
		traverse_type rv(npos, false);

		const char *p = const_char_p(mreg.get_address());
		auto cs = nodes[n].children(p);
		auto cn = nodes[n].num_children(p);

		typename node_type::children_type child = findchild(p, nodes, cs, cn, c);
		if (child != cs + cn) {
			rv.first = *child;
			rv.second = nodes[rv.first].terminal(p);
		}

		return rv;
	}

	traverse_type traverse_end() const {
		traverse_type rv(npos, false);
		return rv;
	}

	browser browse(size_t n=npos) const {
		return browser(this, static_cast<Count>(n));
	}
};

}

#endif
