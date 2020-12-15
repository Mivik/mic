
#ifndef __MIC_GRAPH_H_
#define __MIC_GRAPH_H_

#include <algorithm>
#include <cassert>
#include <string>
#include <type_traits>
#include <vector>

namespace mic {
namespace graph {

template<class edge_info, bool directed = false>
struct base_graph {
#define for_info template<bool local = has_info, std::enable_if_t<local, int> = 0>
#define for_no_info template<bool local = has_info, std::enable_if_t<!local, int> = 0>

	static constexpr bool has_info = !std::is_void_v<edge_info>;
	using edge_type = std::conditional_t<has_info, std::pair<size_t, edge_info>, size_t>;
	using node_t = size_t;
private:
	template<class T>
	struct empty_string_helper { inline std::string operator()(const T &t) { return ""; } };
protected:
	std::vector<std::vector<edge_type>> arr;
public:
	inline size_t size() const { return arr.size(); }
	inline bool empty() const { return arr.empty(); }
	inline void clear() { arr.clear(); }
	inline void resize(size_t count) { clear(); arr.resize(count); }
	inline std::vector<edge_type>& edges(node_t node) { return arr[node]; }
	inline const std::vector<edge_type>& edges(node_t node) const { return arr[node]; }
	for_info inline std::vector<node_t> adjacents(node_t node) const {
		std::vector<node_t> ret; ret.resize(arr[node].size());
		std::transform(arr[node].begin(), arr[node].end(), ret.begin(),
			[](const edge_type &e) { return e.first; });
		return ret;
	}
	for_no_info inline std::vector<node_t> adjacents(node_t node) const { return arr[node]; }
	for_info inline void link(node_t x, node_t y,
		std::conditional_t<has_info, edge_info, char /*trick the compiler*/> info) {
		arr[x].emplace_back(y, info);
		if (!directed && x != y) arr[y].emplace_back(x, info);
	}
	for_no_info inline void link(node_t x, node_t y) {
		arr[x].push_back(y);
		if (!directed && x != y) arr[y].push_back(x);
	}
	template<class string_helper = empty_string_helper<edge_info>>
	std::string to_dot() const {
		const std::string dash = directed? " -> ": " -- ";
		std::string ret = directed? "digraph {": "graph {";
		const auto add = [&ret](const std::string &t) { ret += "\n  "; ret += t; };
		const size_t n = arr.size();
		string_helper helper;
		for (node_t i = 0; i < n; ++i) {
			if constexpr (has_info) {
				for (const auto &[v, info] : arr[i])
					if (v >= i) add(std::to_string(i) + dash + std::to_string(v) + " [label=\"" + helper(info) + "\"]");
			} else {
				for (auto v : arr[i])
					if (v >= i) add(std::to_string(i) + dash + std::to_string(v));
			}
		}
		ret += "\n}";
		return ret;
	}

#undef for_info
#undef for_no_info
};

template<class V = int> struct directed_weighted_graph;
struct directed_graph;
template<class V = int> struct undirected_weighted_graph;
struct undirected_graph;
template<class V = int>
struct weighted_tree;
struct tree;

template<class V> struct directed_weighted_graph : public base_graph<V, true> {
	using typename base_graph<V, true>::node_t;
};
struct directed_graph : public directed_weighted_graph<void> {
	using typename directed_weighted_graph<void>::node_t;
};
template<class V>
struct undirected_weighted_graph : public base_graph<V, false> {
	using typename base_graph<V, false>::node_t;
	inline bool is_tree() const {
		if (this->empty()) return false;
		const size_t n = this->size();
		size_t counter = (n - 1) << 1;
		for (node_t i = 0; i < n; ++i) {
			if (this->arr[i].empty()) return false;
			counter -= this->arr[i].size();
			if (counter < 0) return false;
		}
		return !counter;
	}
	inline const weighted_tree<V>& as_tree() const {
		assert(is_tree());
		return *reinterpret_cast<weighted_tree<V>*>(this);
	}
};
struct undirected_graph : public undirected_weighted_graph<void> {
	using typename undirected_weighted_graph<void>::node_t;
};

template<class V>
struct weighted_tree : public undirected_weighted_graph<V> {
	using typename undirected_weighted_graph<V>::node_t;
private:
	void get_dfs_sequence(node_t x, node_t f, node_t *dst) const {
		*dst++ = x;
		if constexpr (base_graph<V, false>::has_info) {
			for (const auto &[v, _] : this->edges(x))
				if (v != f) get_dfs_sequence(v, x, dst);
		} else {
			for (auto v : this->edges(x))
				if (v != f) get_dfs_sequence(v, x, dst);
		}
	}
	void get_parents(node_t x, node_t f, node_t *dst) const {
		dst[x] = f;
		if constexpr (base_graph<V, false>::has_info) {
			for (const auto &[v, _] : this->edges(x))
				if (v != f) get_parents(v, x, dst);
		} else {
			for (auto v : this->edges(x))
				if (v != f) get_parents(v, x, dst);
		}
	}
public:
	void get_dfs_sequence(node_t root, node_t *dst) const { get_dfs_sequence(root, -1, dst); }
	std::vector<node_t> dfs_sequence(node_t root) const {
		std::vector<node_t> ret; ret.resize(this->size());
		get_dfs_sequence(root, ret.data());
		return ret;
	}
	void get_parents(node_t root, node_t *dst) const { get_parents(root, -1, dst); }
	std::vector<node_t> parents(node_t root) const {
		std::vector<node_t> ret; ret.resize(this->size());
		get_parents(root, ret.data());
		return ret;
	}
	inline std::vector<node_t> prufer_code() const {
		const size_t n = this->size();
		auto pa = parents(n - 1);
		node_t ptr = -1;
		std::vector<size_t> deg(n);
		for (node_t i = 0; i < n; ++i) {
			deg[i] = this->edges(i).size();
			if (deg[i] == 1 && ptr == -1) ptr = i;
		}
		std::vector<int> ret(n - 2);
		int leaf = ptr;
		for (int &r : ret) {
			r = pa[leaf];
			if (--deg[r] == 1 && r < ptr) leaf = r;
			else {
				while (deg[++ptr] != 1);
				leaf = ptr;
			}
		}
		return ret;
	}
};
struct tree : public weighted_tree<void> {
	using typename weighted_tree<void>::node_t;
	static tree from_prufer_code(const std::vector<node_t> &prufer) {
		const size_t n = prufer.size() + 2;
		tree ret; ret.resize(n);
		std::vector<size_t> deg(n, 1);
		for (int v : prufer) ++deg[v];
		int ptr = -1;
		while (deg[++ptr] != 1);
		int leaf = ptr;
		for (int i = 0; i < n - 2; ++i) {
			const int x = prufer[i]; ret.link(leaf, x);
			if (--deg[x] == 1 && x < ptr) leaf = x;
			else {
				while (deg[++ptr] != 1);
				leaf = ptr;
			}
		}
		ret.link(leaf, n - 1);
		return ret;
	}
};
struct binary_tree {
	using node_t = size_t;
private:
	std::vector<node_t> ls, rs;
public:
	// Construct a binary tree from a valid brackets sequence.
	static binary_tree from_brackets(const std::string &str) {
		assert(!(str.size() & 1));
		binary_tree ret; ret.resize(str.size() >> 1);
		std::vector<node_t> stack;
		node_t lst = 0, top = 0;
		bool insert_right = false;
		for (char c : str) {
			assert(c == '(' || c == ')');
			if (c == '(') {
				const node_t pre = lst; stack.push_back(lst = top++);
				if (lst) ret.set_son(pre, std::exchange(insert_right, false), lst);
			} else {
				lst = stack.back(); stack.pop_back();
				insert_right = true;
			}
		}
		return ret;
	}

	inline size_t size() const { return ls.size(); }
	inline void resize(size_t n) { ls.clear(); rs.clear(); ls.resize(n, -1); rs.resize(n, -1); }
	inline node_t left_son(node_t x) const { return ls[x]; }
	inline node_t right_son(node_t x) const { return rs[x]; }
	inline node_t son(node_t x, bool right) const { return (right? rs: ls)[x]; }
	inline std::pair<node_t, node_t> sons(node_t x) const { return { ls[x], rs[x] }; }
	inline void set_left_son(node_t x, node_t v) { ls[x] = v; }
	inline void set_right_son(node_t x, node_t v) { rs[x] = v; }
	inline void set_son(node_t x, bool right, node_t v) { (right? rs: ls)[x] = v; }
	inline void set_sons(node_t x, const std::pair<node_t, node_t> &pa) { ls[x] = pa.first; rs[x] = pa.second; }
	inline tree to_tree() const {
		tree ret; ret.resize(size());
		for (node_t i = size() - 1; ~i; --i) {
			if (~ls[i]) ret.link(i, ls[i]);
			if (~rs[i]) ret.link(i, rs[i]);
		}
		return ret;
	}
};

} // namespace graph
} // namespace mic

#endif
