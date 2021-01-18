
#ifndef __MIC_RANDOM_H_
#define __MIC_RANDOM_H_

#include <algorithm>
#include <cassert>
#include <limits>
#include <random>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "graph.h"

namespace mic {

template<class G = std::mt19937>
struct random_engine {
	static const size_t CHOOSE_USE_SPARSE_THRESOLD = 1024;
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

	// We don't guarantee that the result is sorted.
	template<class T, class = std::enable_if_t<std::is_integral_v<T>>>
	inline std::vector<T> choose(T lo, T hi, size_t num) {
		if (!num) return {};
		assert(lo <= hi);
		const auto len = hi - lo + 1;
		assert(len >= num);
		if (len < CHOOSE_USE_SPARSE_THRESOLD) {
			short tmp[len]; std::iota(tmp, tmp + len, 0);
			shuffle(tmp, tmp + len);
			std::vector<T> ret; ret.resize(num);
			for (size_t i = 0; i < num; ++i) ret[i] = lo + tmp[i];
			return ret;
		} else {
			std::unordered_map<T, T> rest;
			T tmp[num];
			for (T i = 0; i < num; ++i) tmp[i] = lo + i;
			for (T i = 0; i < num; ++i) {
				const T j = rand(i, len - 1);
				if (j < num) std::swap(tmp[i], tmp[j]);
				else {
					auto it = rest.find(j);
					if (it == rest.end()) {
						rest[j] = tmp[i];
						tmp[i] = lo + j;
					} else std::swap(tmp[i], it->second);
				}
			}
			return std::vector<T>(tmp, tmp + num);
		}
	}

#define DEFINE_CHOOSE \
	template<class T> \
	inline typename std::iterator_traits<T>::reference choose

	DEFINE_CHOOSE(T first, T last, std::input_iterator_tag) {
		auto ret = first; ++first;
		size_t i = 0;
		while (first != last) {
			if (!rand<size_t>(0, ++i)) ret = first;
			++first;
		}
		return *ret;
	}
	DEFINE_CHOOSE(T first, T last, std::random_access_iterator_tag) {
		return *(first + rand<size_t>(0, last - first - 1));
	}
	DEFINE_CHOOSE(T first, T last) {
		assert(first != last);
		return choose(first, last, typename std::iterator_traits<T>::iterator_category());
	}

#undef DEFINE_CHOOSE

#define DEFINE_CHOOSE \
	template<class InputIt, class OutputIt> \
	inline void choose

	DEFINE_CHOOSE(InputIt first, InputIt last, size_t count, OutputIt result, std::input_iterator_tag) {
		std::vector<InputIt> ret; ret.resize(count);
		size_t i = 0;
		while (first != last && i != count) {
			ret[i] = first;
			++first; ++i;
		}
		assert(i == count && "Input elements are not enough");
		while (first != last) {
			const size_t pos = rand<size_t>(0, i++);
			if (pos < count) ret[pos] = first;
			++first;
		}
		for (auto &it : ret) { *result = *it; ++it; }
	}
	DEFINE_CHOOSE(InputIt first, InputIt last, size_t count, OutputIt result, std::random_access_iterator_tag) {
		for (size_t pos : choose<size_t>(0, last - first - 1, count)) {
			*result = *(first + pos);
			++result;
		}
	}
	DEFINE_CHOOSE(InputIt first, InputIt last, size_t count, OutputIt result) {
		assert(count > 0);
		choose(first, last, count, result, typename std::iterator_traits<InputIt>::iterator_category());
	}

#undef DEFINE_CHOOSE

	template<class T, class = std::enable_if_t<std::is_integral_v<T>>>
	inline std::vector<T> partition(T sum, T count, T min_value = 1) {
		if (min_value < 0) min_value = 0;
		assert(sum >= 0 && count > 0);
		assert(min_value * count <= sum);
		const T len = sum + count * (1 - min_value) - 1;
		auto ps = choose<T>(0, len - 1, count - 1);
		std::sort(ps.begin(), ps.end());
		std::vector<T> ret; ret.resize(count);
		T last = 0;
		for (size_t i = 0; i < ps.size(); ++i) {
			ret[i] = ps[i] - last + min_value;
			last = ps[i] + 1;
		}
		ret.back() = len - last + min_value;
		return ret;
	}

	inline graph::tree tree(size_t size) {
		assert(size > 0);
		if (size == 1) {
			graph::tree tr; tr.resize(1);
			return tr;
		}
		std::vector<size_t> prufer; prufer.resize(size - 2);
		for (size_t &v : prufer) v = rand(0UL, size - 1);
		return graph::tree::from_prufer_code(prufer);
	}

	inline std::string brackets(size_t n) {
		const size_t len = n << 1;
		bool arr[len];
		std::fill(arr, arr + n, 1);
		std::fill(arr + n, arr + len, 0);
		shuffle(arr, arr + len);
		size_t start = 0, end = len;
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

					// we swap S and T and then flip S, i and j
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
		std::string ret; ret.resize(len);
		for (size_t i = 0; i < len; ++i) ret[i] = "()"[arr[i]];
		return ret;
	}

	inline graph::binary_tree binary_tree(size_t n) { return graph::binary_tree::from_brackets(brackets(n)); }
};

} // namespace mic

#endif
