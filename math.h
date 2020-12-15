
#ifndef __MIC_MATH_H_
#define __MIC_MATH_H_

typedef long long qe;

extern const int mod;

inline int pro(int x) { return x >= mod? x - mod: x; }
inline int per(int x) { return x < 0? x + mod: x; }
inline qe ksm(qe x, int p = mod - 2) {
	qe ret = 1;
	for (; p; p >>= 1, (x *= x) %= mod) if (p & 1) (ret *= x) %= mod;
	return ret;
}

#endif
