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

/*
References:
- http://en.wikipedia.org/wiki/Trie
- http://www.wutka.com/dawg.html
*/

#pragma once
#ifndef TDC_TRIE_HPP_f28c53c53a48d38efafee7fb7004a01faaac9e22
#define TDC_TRIE_HPP_f28c53c53a48d38efafee7fb7004a01faaac9e22

#include <stdint.h>
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <limits>

namespace tdc {

const char* const TRIE_COPYRIGHT_STRING = "Copyright (C) 2013 Tino Didriksen. All Rights Reserved.";
const uint32_t TRIE_VERSION_MAJOR = 0;
const uint32_t TRIE_VERSION_MINOR = 8;
const uint32_t TRIE_VERSION_PATCH = 0;
const uint32_t TRIE_REVISION = 9423;

typedef std::basic_string<uint8_t> u8string;
typedef std::basic_string<uint16_t> u16string;
typedef std::basic_string<uint32_t> u32string;

template<typename T>
struct trie_serializer {
	void serialize(std::ostream& out, const T& value) const {
		out.write(reinterpret_cast<const char*>(&value), sizeof(T));
	}

	T unserialize(std::istream& in) const {
		T value;
		in.read(reinterpret_cast<char*>(&value), sizeof(T));
		return value;
	}
};

//*
template<typename T, typename Y>
typename T::iterator findchild(T& t, const Y& y) {
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
	if (first != t.end() && first->first != y) {
		first = t.end();
	}
	return first;
}

template<typename T, typename Y>
typename T::const_iterator findchild(const T& t, const Y& y) {
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

/*/

template<typename T, typename Y>
typename T::iterator findchild(T& t, const Y& y) {
	typename T::iterator it;
	for (it = t.begin() ; it != t.end() ; ++it) {
		if (it->first == y) {
			break;
		}
	}
	return it;
}

template<typename T, typename Y>
typename T::const_iterator findchild(const T& t, const Y& y) {
	typename T::const_iterator it;
	for (it = t.begin() ; it != t.end() ; ++it) {
		if (it->first == y) {
			break;
		}
	}
	return it;
}
//*/

template<typename T, typename Y>
void insertchild(T& t, const Y& y) {
	typename T::iterator it;
	for (it = t.begin() ; it != t.end() ; ++it) {
		if (it->first == y.first) {
			return;
		}
		if (it->first > y.first) {
			break;
		}
	}
	t.insert(it, y);
}

template<typename String, typename Count=uint32_t, typename Serializer=trie_serializer<typename String::value_type> >
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
		size_t children_depth;
		Count parent;
		children_type children;

		void buildString(const query_path_type& qp, String& in) const {
			in.reserve(qp.size());
			for (typename query_path_type::const_iterator it = qp.begin() ; it != qp.end() ; ++it) {
				in.push_back((*it)->self);
			}
		}

	public:

		trie_node() :
		parent(std::numeric_limits<Count>::max())
		{
		}

		trie_node(Count parent, typename String::value_type self) :
		terminal(false),
		self(self),
		children_depth(0),
		parent(parent)
		{
		}

		bool add(root_type& root, const String& entry, size_t pos=0, size_t depth=0) {
			if (pos < entry.size()) {
				typename children_type::iterator child = findchild(children, entry[pos]);
				if (child != children.end()) {
					node_type& node = root.nodes[child->second];
					return node.add(root, entry, pos+1, depth+1);
				}
				else {
					Count p = static_cast<Count>(this - &root.nodes[0]);
					Count z = static_cast<Count>(root.nodes.size());
					insertchild(children, std::make_pair(entry[pos], z));
					root.nodes.resize(z+1);
					root.nodes.back() = node_type(p, entry[pos]);
					root.nodes.back().add(root, entry, pos+1, depth+1);
					return true;
				}
			}
			else {
				if (!terminal) {
					terminal = true;
					updateChildrenDepth(root);
					return true;
				}
			}
			return false;
		}

		void query(const root_type& root, const String& entry, size_t pos, query_type& collected, query_path_type& qp, size_t maxdist=0, size_t curdist=0, size_t depth=0) const {
			qp.push_back(this);

			if (pos < entry.size()) {
				typename children_type::const_iterator child = findchild(children, entry[pos]);
				if (child != children.end()) {
					root.nodes[child->second].query(root, entry, pos+1, collected, qp, maxdist, curdist, depth+1);
				}
			}

			if (curdist < maxdist) {
				for (typename children_type::const_iterator child = children.begin() ; child != children.end() ; ++child) {
					if (pos >= entry.size() || child->first != entry[pos]) {
						root.nodes[child->second].query(root, entry, pos, collected, qp, maxdist, curdist+1, depth+1);
						root.nodes[child->second].query(root, entry, pos+1, collected, qp, maxdist, curdist+1, depth+1);
					}
					for (size_t i = 1 ; pos+i < entry.size() ; ++i) {
						if (child->first == entry[pos+i]) {
							root.nodes[child->second].query(root, entry, pos+i+1, collected, qp, maxdist, curdist+i, depth+1);
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
		void updateChildrenDepth(root_type& root, size_t depth=0) {
			children_depth = std::max(children_depth, depth);
			if (parent != std::numeric_limits<Count>::max()) {
				root.nodes[parent].updateChildrenDepth(root, depth+1);
			}
		}

		bool equals(const root_type& root, const node_type* second) const {
			if (self != second->self) {
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
	typedef std::vector<std::pair<typename String::value_type, Count> > children_type;
	typedef std::vector<const node_type*> query_path_type;

	Serializer serializer;
	bool compressed;
	node_container_type nodes;
	children_type children;

public:
	class const_iterator {
	private:
		friend class trie;
		const trie *owner;
		std::vector<Count> path;

	public:
		const_iterator() :
		owner(0),
		path(1)
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
			for (size_t i=0 ; i<path.size() ; ++i) {
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
					typename trie::children_type::const_iterator child = findchild(owner->children, owner->nodes[old].self);
					++child;
					if (child != owner->children.end()) {
						Count n = child->second;
						path.push_back(n);
						while (!owner->nodes[n].children.empty()) {
							n = owner->nodes[n].children.front().second;
							path.push_back(n);
						}
						goto plus_return;
					}
				}
				else {
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
			}
		plus_return:
			return *this;
		}
	};

	friend class const_iterator;

	typedef std::map<String,size_t> query_type;
	typedef std::pair<size_t,bool> traverse_type;
	typedef String value_type;

	trie() : compressed(false) {
	}

	void serialize(std::ostream& out) const {
		const char *trie = "TRIE";
		out.write(trie, 4);
		uint8_t z = sizeof(typename String::value_type);
		out.write(reinterpret_cast<const char*>(&z), sizeof(z));

		out.write(reinterpret_cast<const char*>(&compressed), sizeof(compressed));

		Count value = static_cast<Count>(nodes.size());
		out.write(reinterpret_cast<const char*>(&value), sizeof(value));
		for (size_t n=0 ; n<nodes.size() ; ++n) {
			serializer.serialize(out, nodes[n].self);
		}
		for (size_t n=0 ; n<nodes.size() ; ++n) {
			out.write(reinterpret_cast<const char*>(&nodes[n].terminal), sizeof(nodes[n].terminal));
		}
		for (size_t n=0 ; n<nodes.size() ; ++n) {
			Count value = static_cast<Count>(nodes[n].children.size());
			out.write(reinterpret_cast<const char*>(&value), sizeof(value));
		}
		for (size_t n=0 ; n<nodes.size() ; ++n) {
			for (size_t c=0 ; c<nodes[n].children.size() ; ++c) {
				out.write(reinterpret_cast<const char*>(&nodes[n].children[c].second), sizeof(nodes[n].children[c].second));
			}
		}

		value = static_cast<Count>(children.size());
		out.write(reinterpret_cast<const char*>(&value), sizeof(value));
		for (size_t c=0 ; c<children.size() ; ++c) {
			out.write(reinterpret_cast<const char*>(&children[c].second), sizeof(children[c].second));
		}
	}

	void unserialize(std::istream& in) {
		clear();

		std::string trie(4,0);
		in.read(&trie[0], 4);
		if (trie != "TRIE") {
			throw -1;
		}

		uint8_t s;
		in.read(reinterpret_cast<char*>(&s), sizeof(s));
		if (s != sizeof(typename String::value_type)) {
			throw -1;
		}

		in.read(reinterpret_cast<char*>(&compressed), sizeof(compressed));

		Count z;
		in.read(reinterpret_cast<char*>(&z), sizeof(z));
		nodes.resize(z);
		for (size_t n=0 ; n<z ; ++n) {
			nodes[n].self = serializer.unserialize(in);
		}
		for (size_t n=0 ; n<z ; ++n) {
			in.read(reinterpret_cast<char*>(&nodes[n].terminal), sizeof(nodes[n].terminal));
		}
		for (size_t n=0 ; n<z ; ++n) {
			Count c;
			in.read(reinterpret_cast<char*>(&c), sizeof(c));
			nodes[n].children.resize(c);
		}
		for (size_t n=0 ; n<z ; ++n) {
			for (size_t c=0 ; c<nodes[n].children.size() ; ++c) {
				in.read(reinterpret_cast<char*>(&nodes[n].children[c].second), sizeof(nodes[n].children[c].second));
				nodes[n].children[c].first = nodes[nodes[n].children[c].second].self;
				nodes[nodes[n].children[c].second].parent = static_cast<Count>(n);
			}
		}

		in.read(reinterpret_cast<char*>(&z), sizeof(z));
		children.resize(z);
		for (size_t c=0 ; c<children.size() ; ++c) {
			in.read(reinterpret_cast<char*>(&children[c].second), sizeof(children[c].second));
			children[c].first = nodes[children[c].second].self;
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
		children.clear();
	}

	const_iterator begin() const {
		return const_iterator(this, 0);
	}

	const_iterator end() const {
		return const_iterator(this, static_cast<Count>(nodes.size()));
	}

	bool add(const String& entry) {
		if (entry.empty()) {
			return false;
		}
		if (compressed) {
			return false;
		}
		typename children_type::iterator child = findchild(children, entry[0]);
		if (child != children.end()) {
			node_type& node = nodes[child->second];
			return node.add(*this, entry, 1, 1);
		}
		Count z = static_cast<Count>(nodes.size());
		insertchild(children, std::make_pair(entry[0], z));
		nodes.resize(z+1);
		nodes.back() = node_type(std::numeric_limits<Count>::max(), entry[0]);
		nodes.back().add(*this, entry, 1, 1);
		return true;
	}

	void insert(const String& entry) {
		add(entry);
	}

	query_type query(const String& entry, size_t maxdist = 0) const {
		query_type matches;
		if (!entry.empty()) {
			query_path_type qp;
			qp.reserve(entry.size()+maxdist+2);

			typename children_type::const_iterator child = findchild(children, entry[0]);
			if (child != children.end()) {
				nodes[child->second].query(*this, entry, 1, matches, qp, maxdist);
			}
			if (maxdist) {
				for (typename children_type::const_iterator child = children.begin() ; child != children.end() ; ++child) {
					if (child->first != entry[0]) {
						nodes[child->second].query(*this, entry, 0, matches, qp, maxdist, 1);
						nodes[child->second].query(*this, entry, 1, matches, qp, maxdist, 1);
					}
					for (size_t i = 1 ; i < entry.size() ; ++i) {
						if (child->first == entry[i]) {
							nodes[child->second].query(*this, entry, i+1, matches, qp, maxdist, i);
						}
					}
				}
			}
		}
		return matches;
	}

	const_iterator find(const String& entry) const {
		const_iterator rv = end();
		typename children_type::const_iterator child = findchild(children, entry[0]);
		if (child != children.end()) {
			rv.path.clear();
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

	traverse_type traverse(typename String::value_type c, size_t n) const {
		traverse_type rv(std::numeric_limits<size_t>::max(), false);

		if (n == std::numeric_limits<size_t>::max()) {
			typename children_type::const_iterator child = findchild(children, c);
			if (child != children.end()) {
				rv.first = child->second;
				rv.second = nodes[rv.first].terminal;
			}
		}
		else {
			typename children_type::const_iterator child = findchild(nodes[n].children, c);
			if (child != nodes[n].children.end()) {
				rv.first = child->second;
				rv.second = nodes[rv.first].terminal;
			}
		}

		return rv;
	}

	traverse_type traverse(typename String::value_type c) const {
		return traverse(c, std::numeric_limits<size_t>::max());
	}

	void compress() {
		if (compressed) {
			return;
		}

		typedef std::multimap<typename String::value_type,node_type*> multichild_type;
		typedef std::map<size_t,multichild_type> depths_type;
		depths_type depths;

		for (size_t i=0 ; i<nodes.size() ; ++i) {
			depths[nodes[i].children_depth].insert(std::make_pair(nodes[i].self, &nodes[i]));
		}

		std::cerr << "Compressing " << nodes.size() << " nodes..." << std::endl;

		size_t removed = 0;

		for (size_t i=0 ; ; ++i) {
			if (depths.find(i) == depths.end()) {
				break;
			}
			std::cerr << "Handling depth " << i << "..." << std::flush;

			multichild_type& mchild = depths[i];
			bool did_compress = false;

			typename multichild_type::iterator onode;
			for (onode = mchild.begin() ; onode != mchild.end() ; ++onode) {
				typename multichild_type::iterator inode = onode;
				++inode;
				for ( ; inode != mchild.end() ; ) {
					node_type *first = onode->second;
					node_type *second = inode->second;
					if (first->self != second->self) {
						break;
					}
					if (second->parent == std::numeric_limits<Count>::max() || !first->equals(*this, second)) {
						++inode;
						continue;
					}

					findchild(nodes[second->parent].children, first->self)->second = static_cast<Count>(first - &nodes[0]);
					second->parent = std::numeric_limits<Count>::max();
					second->self = typename String::value_type();
					second->children.clear();
					second->children_depth = 0;
					mchild.erase(inode++);
					did_compress = true;
					++removed;
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
			if (nodes[i].parent != std::numeric_limits<Count>::max() || nodes[i].self != typename String::value_type()
				|| nodes[i].children.size() != 0 || nodes[i].children_depth != 0) {
				oldnew[i] = static_cast<Count>(tosave.size());
				tosave.push_back(nodes[i]);
			}
		}

		nodes.swap(tosave);
		for (size_t i=0 ; i<nodes.size() ; ++i) {
			nodes[i].parent = oldnew[nodes[i].parent];
			for (size_t c=0 ; c<nodes[i].children.size() ; ++c) {
				nodes[i].children[c].second = oldnew[nodes[i].children[c].second];
			}
		}
		for (size_t c=0 ; c<children.size() ; ++c) {
			children[c].second = oldnew[children[c].second];
		}
		std::cerr << std::endl;
	}
};

}

#endif
