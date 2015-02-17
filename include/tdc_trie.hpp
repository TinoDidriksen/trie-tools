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

/*
References:
- http://en.wikipedia.org/wiki/Trie
- http://www.wutka.com/dawg.html
*/

#pragma once
#ifndef TDC_TRIE_HPP_f28c53c53a48d38efafee7fb7004a01faaac9e22
#define TDC_TRIE_HPP_f28c53c53a48d38efafee7fb7004a01faaac9e22

#include <boost/typeof/typeof.hpp>
#include <boost/foreach.hpp>

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

const char* const TRIE_COPYRIGHT_STRING = "Copyright (C) 2013-2015 Tino Didriksen <mail@tinodidriksen.com>. Licensed under GPLv3+";
const uint32_t TRIE_VERSION_MAJOR = 0;
const uint32_t TRIE_VERSION_MINOR = 8;
const uint32_t TRIE_VERSION_PATCH = 2;
const uint32_t TRIE_REVISION = 10503;
const uint32_t TRIE_SERIALIZED_REVISION = 10498;

typedef std::basic_string<uint8_t> u8string;
typedef std::basic_string<uint16_t> u16string;
typedef std::basic_string<uint32_t> u32string;

inline bool isspace(char c) {
	return (c == 0x20 || c == 0x09 || c == 0x0A || c == 0x0D);
}

template<typename T>
inline const char *const_char_p(T t) {
	return reinterpret_cast<const char*>(t);
}

template<typename T>
inline char *char_p(T t) {
	return reinterpret_cast<char*>(t);
}

template<typename T, typename Y>
inline typename T::iterator lower_bound(T& t, const Y& y) {
	typename T::iterator it, first = t.begin();
	size_t count = t.size(), step;

	while (count > 0) {
		it = first;
		step = count / 2;
		it += step;
		if (it->first < y) {
			first = ++it;
			count -= step + 1;
		}
		else {
			count = step;
		}
	}
	return first;
}

template<typename T, typename Y>
inline typename T::iterator findchild(T& t, const Y& y) {
	typename T::iterator first = lower_bound(t, y);

	if (first != t.end() && first->first != y) {
		first = t.end();
	}
	return first;
}

template<typename T, typename Y>
inline typename T::const_iterator findchild(const T& t, const Y& y) {
	typename T::const_iterator it, first = t.begin();
	size_t count = t.size(), step;

	while (count > 0) {
		it = first;
		step = count / 2;
		it += step;
		if (it->first < y) {
			first = ++it;
			count -= step + 1;
		}
		else {
			count = step;
		}
	}
	if (first != t.end() && first->first != y) {
		first = t.end();
	}
	return first;
}

template<typename T, typename Y>
inline typename T::iterator insertchild(T& t, Y&& y) {
	typename T::iterator it;
	for (it = t.begin() ; it != t.end() ; ++it) {
		if (it->first == y.first) {
			return it;
		}
		if (it->first > y.first) {
			break;
		}
	}
	return t.insert(it, std::move(y));
}

template<typename String=u16string, typename Count=uint32_t>
class trie {
private:

	class trie_node {
	protected:
		friend class trie;

		typedef trie_node node_type;
		typedef std::vector<std::pair<typename String::value_type, Count> > children_type;
		typedef std::vector<const node_type*> query_path_type;
		typedef std::map<String,size_t> query_type;
		typedef trie root_type;

		bool terminal;
		typename String::value_type self;
		Count num_terminals;
		Count children_depth;
		children_type children;

		void buildString(const query_path_type& qp, String& in) const {
			in.reserve(qp.size());
			for (typename query_path_type::const_iterator it = qp.begin() ; it != qp.end() ; ++it) {
				in.push_back((*it)->self);
			}
		}

	public:

		trie_node(typename String::value_type self = typename String::value_type()) :
		terminal(false),
		self(self),
		num_terminals(0),
		children_depth(0)
		{
		}

		bool add(root_type& root, const String& entry, size_t pos=0) {
			size_t self = this - &root.nodes.front();
			bool rv = false;
			children_depth = std::max(children_depth, static_cast<Count>(entry.size() - pos));
			if (pos < entry.size()) {
				typename children_type::iterator child = findchild(children, entry[pos]);
				if (child != children.end()) {
					node_type& node = root.nodes[child->second];
					rv = node.add(root, entry, pos+1);
				}
				else {
					Count z = static_cast<Count>(root.nodes.size());
					insertchild(children, std::make_pair(entry[pos], z));
					root.nodes.resize(z+1);
					root.nodes.back() = node_type(entry[pos]);
					rv = root.nodes.back().add(root, entry, pos+1);
				}
			}
			else {
				if (!terminal) {
					terminal = true;
					rv = true;
				}
			}
			if (rv) {
				++root.nodes[self].num_terminals;
			}
			return rv;
		}

		void query(const root_type& root, const String& entry, size_t pos, query_type& collected, query_path_type& qp, size_t maxdist=0, size_t curdist=0) const {
			qp.push_back(this);

			if (pos < entry.size()) {
				typename children_type::const_iterator child = findchild(children, entry[pos]);
				if (child != children.end()) {
					root.nodes[child->second].query(root, entry, pos+1, collected, qp, maxdist, curdist);
				}
			}

			if (curdist < maxdist) {
				for (typename children_type::const_iterator child = children.begin() ; child != children.end() ; ++child) {
					if (pos >= entry.size() || child->first != entry[pos]) {
						root.nodes[child->second].query(root, entry, pos, collected, qp, maxdist, curdist+1);
						root.nodes[child->second].query(root, entry, pos+1, collected, qp, maxdist, curdist+1);
					}
					for (size_t i = 1 ; pos+i < entry.size() ; ++i) {
						if (child->first == entry[pos+i]) {
							root.nodes[child->second].query(root, entry, pos+i+1, collected, qp, maxdist, curdist+i);
						}
					}
				}
			}

			if (terminal) {
				size_t dist = curdist;
				if (pos < entry.size()) {
					dist += entry.size() - pos;
				}
				else {
					dist += pos - entry.size();
				}
				if (dist <= maxdist) {
					String out;
					buildString(qp, out);
					typename query_type::iterator ins = collected.insert(std::make_pair(out, dist)).first;
					ins->second = std::min(ins->second, dist);
				}
			}

			qp.pop_back();
		}

	private:
		bool equals(const root_type& root, const node_type* second) const {
			if (self != second->self) {
				return false;
			}
			if (num_terminals != second->num_terminals) {
				return false;
			}
			if (children_depth != second->children_depth) {
				return false;
			}
			if (terminal != second->terminal) {
				return false;
			}
			if (children.size() != second->children.size()) {
				return false;
			}
			for (typename children_type::const_iterator mine = children.begin(), other = second->children.begin()
				; mine != children.end() && other != second->children.end() ; ++mine, ++other) {
				const node_type *nmine = &root.nodes[mine->second];
				const node_type *nother = &root.nodes[other->second];
				if (!nmine->equals(root, nother)) {
					return false;
				}
			}
			return true;
		}
	};

	friend class trie_node;

	typedef trie_node node_type;
	typedef std::vector<node_type> node_container_type;
	typedef std::vector<const node_type*> query_path_type;

	bool compressed;
	node_container_type nodes;

public:
	class const_iterator {
	private:
		friend class trie;
		const trie *owner;
		std::vector<Count> path;

	public:
		const_iterator(const trie *owner = 0) :
		owner(owner)
		{
		}

		const_iterator(const trie *owner, Count n) :
		owner(owner),
		path(1, n)
		{
			if (owner && n < owner->nodes.size() && !owner->nodes[n].terminal) {
				while (!owner->nodes[n].children.empty()) {
					n = owner->nodes[n].children.front().second;
					path.push_back(n);
				}
			}
			if (owner && n == owner->nodes.size()) {
				path.clear();
			}
		}

		String operator*() const {
			String rv;
			rv.reserve(path.size());
			for (size_t i=1 ; i<path.size() ; ++i) {
				rv += owner->nodes[path[i]].self;
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

				typename trie_node::children_type::const_iterator child = findchild(owner->nodes[path.back()].children, owner->nodes[old].self);
				++child;
				if (child != owner->nodes[path.back()].children.end()) {
					Count n = child->second;
					path.push_back(n);
					while (!owner->nodes[n].children.empty()) {
						n = owner->nodes[n].children.front().second;
						path.push_back(n);
					}
					goto plus_return;
				}
				if (owner->nodes[path.back()].terminal) {
					break;
				}
			}
		plus_return:
			return *this;
		}
	};

	class browser {
	private:
		const trie *owner;
		Count node;

	public:
		class browser_out {
		private:
			const trie *owner;
			Count node;

		public:
			browser_out(const trie *owner = 0, Count node = npos) :
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
			const trie *owner;
			Count node;
			Count which;

		public:
			browser_iter(const trie *owner = 0, Count node = npos, Count which = 0) :
				owner(owner),
				node(node),
				which(which) {
			}

			browser_out values() const {
				return browser_out(owner, owner->nodes[node].children[which].second);
			}

			std::pair<typename String::value_type, Count> operator*() const {
				const typename trie_node::children_type& children = owner->nodes[node].children;
				return std::make_pair(children[which].first, owner->nodes[children[which].second].num_terminals);
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

		browser(const trie *owner = 0, Count node = npos) :
			owner(owner),
			node(node) {
		}

		browser_iter begin() const {
			return browser_iter(owner, node, 0);
		}

		browser_iter end() const {
			return browser_iter(owner, node, owner->nodes[node].children.size());
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

	trie() :
		compressed(false),
		nodes(1) {
	}

	void serialize(std::ostream& out) const {
		std::vector<Count> ofs(nodes.size());

		const char *trie = "TRIE";
		out.write(trie, 4);
		out.write(const_char_p(&TRIE_SERIALIZED_REVISION), sizeof(TRIE_SERIALIZED_REVISION));

		uint16_t z = sizeof(typename String::value_type);
		out.write(const_char_p(&z), sizeof(z));

		z = compressed;
		out.write(const_char_p(&z), sizeof(z));

		Count value = static_cast<Count>(nodes.size());
		out.write(const_char_p(&value), sizeof(value));
		for (size_t n = 0; n<nodes.size(); ++n) {
			ofs[n] = static_cast<Count>(out.tellp());
			z = nodes[n].self;
			out.write(const_char_p(&z), sizeof(z));
			z = nodes[n].terminal;
			out.write(const_char_p(&z), sizeof(z));
			out.write(const_char_p(&nodes[n].num_terminals), sizeof(nodes[n].num_terminals));

			Count value = static_cast<Count>(nodes[n].children.size());
			out.write(const_char_p(&value), sizeof(value));
			for (size_t c = 0; c<nodes[n].children.size(); ++c) {
				out.write(const_char_p(&nodes[n].children[c].second), sizeof(nodes[n].children[c].second));
			}
		}
		out.write(const_char_p(ofs.data()), ofs.size()*sizeof(Count));
	}

	void unserialize(std::istream& in) {
		clear();

		std::string trie(4, 0);
		in.read(&trie[0], 4);
		if (trie != "TRIE") {
			throw std::runtime_error("Unserialize stream did not start with magic byte sequence TRIE");
		}

		uint32_t rev = 0;
		in.read(char_p(&rev), sizeof(rev));
		if (rev != TRIE_SERIALIZED_REVISION) {
			char _msg[] = "Unserialize expected revision %u but data had revision %u";
			std::string msg(sizeof(_msg)+11 + 11 + 1, 0);
			msg.resize(sprintf(&msg[0], _msg, TRIE_SERIALIZED_REVISION, rev));
			throw std::runtime_error(msg);
		}

		uint16_t s;
		in.read(char_p(&s), sizeof(s));
		if (s != sizeof(typename String::value_type)) {
			char _msg[] = "Unserialize expected code unit width %u but data had width %u";
			std::string msg(sizeof(_msg)+11 + 11 + 1, 0);
			msg.resize(sprintf(&msg[0], _msg, sizeof(typename String::value_type), s));
			throw std::runtime_error(msg);
		}

		in.read(char_p(&s), sizeof(s));
		compressed = (s != 0);

		Count z;
		in.read(char_p(&z), sizeof(z));
		nodes.resize(z);
		for (size_t n = 0; n < z; ++n) {
			in.read(char_p(&s), sizeof(s));
			nodes[n].self = static_cast<typename String::value_type>(s);
			in.read(char_p(&s), sizeof(s));
			nodes[n].terminal = (s != 0);
			in.read(char_p(&nodes[n].num_terminals), sizeof(nodes[n].num_terminals));

			Count c;
			in.read(char_p(&c), sizeof(c));
			nodes[n].children.resize(c);
			for (size_t c = 0; c < nodes[n].children.size(); ++c) {
				in.read(char_p(&nodes[n].children[c].second), sizeof(nodes[n].children[c].second));
			}
		}
		for (size_t n = 0; n < z; ++n) {
			for (size_t c = 0; c < nodes[n].children.size(); ++c) {
				nodes[n].children[c].first = nodes[nodes[n].children[c].second].self;
			}
		}
	}

	bool is_compressed() const {
		return compressed;
	}

	size_t size() const {
		return nodes.size();
	}

	void clear() {
		compressed = false;
		nodes.clear();
		nodes.resize(1);
	}

	const_iterator begin() const {
		return const_iterator(this, 0);
	}

	const_iterator end() const {
		return const_iterator(this);
	}

	bool add(const String& entry) {
		if (entry.empty()) {
			return false;
		}
		if (compressed) {
			return false;
		}
		return nodes[0].add(*this, entry);
	}

	void insert(const String& entry) {
		add(entry);
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
		typename node_type::children_type::const_iterator child = findchild(nodes[0].children, entry[0]);
		if (child != nodes[0].children.end()) {
			rv.path.clear();
			rv.path.push_back(0);
			rv.path.push_back(child->second);
			for (size_t i=1 ; i<entry.size() ; ++i) {
				Count second = child->second;
				child = findchild(nodes[second].children, entry[i]);
				if (child == nodes[second].children.end()) {
					rv = end();
					break;
				}
				rv.path.push_back(child->second);
			}
			if (!rv.path.empty() && nodes[rv.path.back()].terminal == false) {
				rv = end();
			}
		}
		return rv;
	}

	void erase(const String& entry) {
		if (compressed) {
			return;
		}
		const_iterator it = find(entry);
		if (it != end()) {
			nodes[it.path.back()].terminal = false;
		}
	}

	traverse_type traverse(typename String::value_type c, size_t n=npos) const {
		traverse_type rv(npos, false);

		typename node_type::children_type::const_iterator child = findchild(nodes[n].children, c);
		if (child != nodes[n].children.end()) {
			rv.first = child->second;
			rv.second = nodes[rv.first].terminal;
		}

		return rv;
	}

	browser browse(size_t n=npos) const {
		return browser(this, static_cast<Count>(n));
	}

	void compress() {
		// ToDo: Add compression ratio to bail out early
		if (compressed) {
			return;
		}

		typedef std::vector<std::pair<typename String::value_type,std::vector<Count>>> multichild_type;
		typedef std::vector<multichild_type> depths_type;
		depths_type depths(nodes[0].children_depth + 1);
		std::vector<Count> parents(nodes.size(), std::numeric_limits<Count>::max());
		size_t max_child = 0;

		for (size_t i=0 ; i<nodes.size() ; ++i) {
			multichild_type& mchild = depths[nodes[i].children_depth];
			BOOST_AUTO(it, lower_bound(mchild, nodes[i].self));
			if (it == mchild.end() || it->first != nodes[i].self) {
				it = mchild.insert(it, std::make_pair(nodes[i].self, std::vector<Count>()));
			}
			it->second.push_back(i);
			max_child = std::max(max_child, nodes[i].children.size());
			BOOST_FOREACH (typename node_type::children_type::value_type& ch, nodes[i].children) {
				parents[ch.second] = i;
			}
		}

		std::cerr << "Compressing " << nodes.size() << " nodes..." << std::endl;
		std::cerr << "Highest children count: " << max_child << std::endl;

		size_t removed = 0;

		BOOST_FOREACH (multichild_type& depth, depths) {
			std::cerr << "Handling depth " << (&depth - &depths[0]) << "..." << std::flush;

			bool did_compress = false;

			BOOST_FOREACH (typename multichild_type::value_type& mchildren, depth) {
				BOOST_AUTO(&mchild, mchildren.second);

				for (BOOST_AUTO(onode, mchild.begin()); onode != mchild.end(); ++onode) {
					if (*onode == 0) {
						continue;
					}
					node_type *first = &nodes[*onode];

					for (BOOST_AUTO(inode, onode + 1); inode != mchild.end(); ++inode) {
						if (*inode == 0) {
							continue;
						}
						node_type *second = &nodes[*inode];
						if (parents[*inode] == std::numeric_limits<Count>::max() || !first->equals(*this, second)) {
							continue;
						}

						findchild(nodes[parents[*inode]].children, first->self)->second = static_cast<Count>(first - &nodes[0]);
						parents[*inode] = std::numeric_limits<Count>::max();
						second->self = typename String::value_type();
						second->children.clear();
						second->children_depth = 0;
						*inode = 0;
						did_compress = true;
						++removed;
					}
				}
			}

			if (did_compress) {
				compressed = true;
				std::cerr << "compressed down to " << nodes.size()-removed << " nodes." << std::endl;
			}
			else {
				std::cerr << "nothing more to do." << std::endl;
				break;
			}
		}

		std::map<Count,Count> oldnew;
		oldnew[std::numeric_limits<Count>::max()] = std::numeric_limits<Count>::max();
		node_container_type tosave;
		tosave.reserve(nodes.size()-removed);

		for (Count i=0 ; i<nodes.size() ; ++i) {
			if (parents[i] != std::numeric_limits<Count>::max() || nodes[i].self != typename String::value_type()
				|| nodes[i].children.size() != 0 || nodes[i].children_depth != 0) {
				oldnew[i] = static_cast<Count>(tosave.size());
				tosave.push_back(nodes[i]);
			}
		}

		nodes.swap(tosave);
		for (size_t i=0 ; i<nodes.size() ; ++i) {
			for (size_t c=0 ; c<nodes[i].children.size() ; ++c) {
				nodes[i].children[c].second = oldnew[nodes[i].children[c].second];
			}
		}
		std::cerr << std::endl;
	}
};

}

#endif
