
#pragma once

#include "io.h"
#include "math.h"

// coco: preserve_begin
struct mint {
	int v;
	mint() {}
	mint(int v): v(v) {}
	explicit mint(qe v): v(v % mod) {}
	inline mint inv() const { return ksm(v); }
	inline mint operator+(const mint &t) const { return pro(v + t.v); }
	inline mint operator-(const mint &t) const { return per(v - t.v); }
	inline mint operator*(const mint &t) const { return (qe)v * t.v % mod; }
	inline mint operator/(const mint &t) const { return (qe)v * t.inv().v % mod; }
	inline void operator+=(const mint &t) { v = pro(v + t.v); }
	inline void operator-=(const mint &t) { v = per(v - t.v); }
	inline void operator*=(const mint &t) { v = (qe)v * t.v % mod; }
	inline void operator/=(const mint &t) { v = (qe)v * t.inv().v % mod; }
	inline bool operator==(const mint &t) const { return v == t.v; }
	inline bool operator!=(const mint &t) const { return v != t.v; }
	friend inline std::ostream& operator<(std::ostream &out, const mint &t) { return out < t.v; }
};
template<> struct Q<mint> { inline void operator()(MI &r, mint &t) { r > t.v; } };
// coco: preserve_end
