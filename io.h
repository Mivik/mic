
#pragma once

#include <cstdio>
#include <cstring>
#include <iostream>

// coco: preserve_begin
#define d isdigit(c)
#define g c=t.get();
#define L return
#define K break
#define A(c,a,b)if(c)a else b;
#define I(c,a)if(c)a;
#define Y goto E
struct MI{private:char bb[1 << 14];FILE*f;char*bs,*be;char e;bool o,l;public:MI():f(stdin),bs(0),be(0){}MI(FILE*f):f(f),bs(0),be(0){}
#ifdef __linux__
MI(const char*s):f(fmemopen(const_cast<char*>(s),strlen(s),"r")){}
#endif
inline operator bool(){L!l;}inline char get(){if(o){o=0;L e;}
#ifdef MIVIK
char c=fgetc(f);I(c==-1,l=1)L c;
#else
I(bs==be,be=(bs=bb)+fread(bb,1,sizeof(bb),f))I(bs==be,{l=1;L-1;})L*bs++;
#endif
}inline void unget(char c){o=1;e=c;}template<class T>inline T read(){T r;*this>r;L r;}template<class T>inline MI&operator>(T&);};template<class T>struct Q{const static bool U=T(-1)>=T(0);inline void operator()(MI&t,T&r)const{r=0;char c;bool y=0;A(U,for(;;){g I(c==-1,Y)I(d,K)},for(;;){g I(c==-1,Y)A(c=='-',{g I(d,{y=1;K;})},I(d,K))})for(;;){I(c==-1,Y)A(d,r=r*10+(c^48);,K)g}t.unget(c);E:;I(y,r=-r)}};template<>struct Q<char>{inline void operator()(MI&t,char&r){int c;for(;;){g I(c==-1,{r=-1;L;})I(!isspace(c),{r=c;L;})}}};template<class T>inline MI&MI::operator>(T&t){Q<T>()(*this,t);L*this;}
#undef d
#undef g
#undef L
#undef K
#undef A
#undef I
#undef Y
template<class T>std::ostream& operator<(std::ostream&out,const T&t){return out<<t;}
#define endl ('\n')
#define P(x) cout<#x" = "<(x)<endl
#define R (cin.read<int>())
// coco: preserve_end

using std::cout;
