
#ifndef __MIC_RANDOM_H_
#define __MIC_RANDOM_H_

#include <algorithm>
#include <cassert>
#include <limits>
#include <random>
#include <type_traits>
#include <vector>

#include "graph.h"

namespace mic {

template<class C, class E>
struct sequence_helper {
	inline size_t size(const C &t) const = delete;
	inline const E& get(const C &t, size_t i) const = delete;
};

template<>
struct sequence_helper<std::string, char> {
	inline size_t size(const std::string &t) const { return t.size(); }
	inline const char& get(const std::string &t, size_t i) const { return t[i]; }
};

template<class T, size_t S>
struct sequence_helper<T[S], T> {
	inline size_t size(const T t[S]) const { return S; }
	inline const T& get(const T t[S], size_t i) const { return t[i]; }
};

template<class G = std::mt19937>
struct random_engine {
private:
	template<class T>
	using distribution_type =
		std::enable_if_t<std::is_arithmetic_v<T>,
			std::conditional_t<
				std::is_integral_v<T>,
				std::uniform_int_distribution<T>,
				std::uniform_real_distribution<T>
			>>;

	G engine;
public:
	random_engine(): engine(std::random_device{}()) {}
	random_engine(typename G::result_type seed): engine(seed) {}
	template<class T>
	inline typename T::result_type dist(T &t) { return t(engine); }
	template<class T>
	inline typename T::result_type dist(T &&t) { return t(engine); }
	template<class T>
	inline T rand(const T &l, const T &r) { return distribution_type<T>(l, r)(engine); }
	template<class T>
	inline T operator()(const T &l, const T &r) { return rand(l, r); }
	template<class T>
	inline void shuffle(T first, T last) { std::shuffle(first, last, engine); }
	inline bool percent(int p) { return operator()(1, 100) <= p; }
	template<class E, class C, class H = sequence_helper<C, E>>
	inline const E& select(const C &source) {
		const H helper;
		return helper.get(source, rand(0UL, helper.size(source) - 1));
	}
	template<class E, class C, class H = sequence_helper<C, E>>
	inline std::vector<E> sequence(const C &source, size_t len) {
		std::vector<E> ret; ret.reserve(len);
		for (size_t i = 0; i < len; ++i) ret.push_back(select(source));
		return ret;
	}
	inline graph::tree tree(size_t size) {
		assert(size > 1);
		std::vector<size_t> prufer; prufer.resize(size - 2);
		for (size_t &v : prufer) v = rand(0UL, size - 1);
		return graph::tree::from_prufer_code(prufer);
	}
	inline std::string brackets(size_t n) {
		assert(!(n & 1));
		bool arr[n];
		const size_t half = n >> 1;
		std::fill(arr, arr + half, 1);
		std::fill(arr + half, arr + n, 0);
		shuffle(arr, arr + n);
		size_t start = 0, end = n;
		while (true) {
			size_t lef_count = 0, rig_count = 0;
			bool ok = true;
			for (size_t i = start, j; i < end; ++i) {
				++(arr[i]? rig_count: lef_count);
				if (lef_count >= rig_count) continue;
				for (j = i + 1; j < end; ++j) {
					++(arr[j]? rig_count: lef_count);
					if (rig_count > lef_count) continue;
					// ( ) ) ) ) ( ( ( ) ) ) ( ( (
					//     i ---S--- j -----T-----

					// We swap S and T and then flip S, i and j
					const size_t len = j - i - 1;
					std::rotate(arr + i + 1, arr + j + 1, arr + end);
					std::copy_backward(arr + end - len - 1, arr + end - 1, arr + end);
					std::transform(arr + end - len, arr + end, arr + end - len, std::logical_not());
					arr[i] = 0; arr[end - len - 1] = 1;
					start = i + 1; end = end - len - 1;
					ok = false;
					break;
				}
			}
			if (ok) break;
		}
		std::string ret; ret.resize(n);
		for (size_t i = 0; i < n; ++i) ret[i] = "()"[arr[i]];
		return ret;
	}
	inline graph::binary_tree binary_tree(size_t n) { return graph::binary_tree::from_brackets(brackets(n << 1)); }
};

} // namespace mic

#endif
