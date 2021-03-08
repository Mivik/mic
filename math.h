
#pragma once

typedef long long qe;

extern const int mod;

inline int add(int x, int y) { return (x += y) >= mod? x - mod: x; }
template<class...Args>
inline int add(int x, int y, Args...args) { return add(add(x, y), args...); }
inline void Add(int &x, int y) { if ((x += y) >= mod) x -= mod; }
inline int sub(int x, int y) { return (x -= y) < 0? x + mod: x; }
inline void Sub(int &x, int y) { if ((x -= y) < 0) x += mod; }
inline qe ksm(qe x, int p = mod - 2) {
	qe ret = 1;
	for (; p; p >>= 1, (x *= x) %= mod) if (p & 1) (ret *= x) %= mod;
	return ret;
}
