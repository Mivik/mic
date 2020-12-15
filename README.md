a
## mic

### graph

```cpp
#include <iostream>
#include <cassert>

#include <mic/graph.h>

int main() {
	using namespace mic::graph;

	{
		directed_weighted_graph<int> g;
		g.resize(3);
		g.link(0, 1, 3);
		g.link(1, 2, 4);
		for (int i = 0; i < 3; ++i)
			for (auto [v, w] : g.edges(i))
				std::cout << i << ' ' << v << " | " << w << std::endl;
		// Output:
		// 0 1 | 3
		// 1 2 | 4
	}

	{
		undirected_graph g;
		g.resize(3);
		g.link(0, 1);
		g.link(1, 2);
		assert(g.is_tree());
	}
}
```

### io

```cpp
#include <mic/io.h>

MI cin;

struct point {
	int x, y;
};
template<> struct Q<point> { inline void operator()(MI &r, point &t) {
	r > t.x > t.y;
} };
int main() {
	cout < (R + R) < endl; // read two numbers and output their sum
	cout < (cin.read<long long>()) < endl; // read a long long and output it
	point pt; cin > pt; // equivalent to "point pt = cin.read<point>();"
	cout < pt.x < ' ' < pt.y < endl;
}
```

### math

```cpp
#include <iostream>

#include <mic/math.h>

// note that this will be used by math.h
const int mod = 10007;

// pro(x): (x >= mod)? x - mod: x
// per(x): (x < 0)? x + mod: x
// ksm(x, p): (x ^ p) % mod
int main() {
	std::cout << pro(10005 + 3) << std::endl; // 1
	std::cout << per(-1) << std::endl; // 10006
	std::cout << ksm(233, 233) << std::endl; // 204
}
```

### mint

A int type modulo `mod`.

```cpp
#include <iostream>

#include <mic/mint.h>

const int mod = 10007;

int main() {
	const int n = 500;
	mint a = 1, b = 1;
	for (int i = 3; i <= n; ++i) {
		const mint c = a + b;
		a = b; b = c;
	}
	// the 500'th element of fibonacci sequence, modulo 10007
	std::cout << b.v << std::endl;

	std::cout << "(57 / 233) % 10007 = " << (mint(57) / 233).v << std::endl; // 2706
}
```

### random

```cpp
#include <iostream>

#include <mic/random.h>

mic::random_engine e; // can also be "mic::random_engine e(seed);"
int main() {
	std::cout << e(0, 23) << std::endl; // a random integer in [0, 23]
	std::cout << e(0., 1.) << std::endl; // a random real
	std::cout << e.rand<long long>(23, 50) << std::endl; // a random long long

	std::cout << e.brackets(10) << std::endl; // a uniformly-random bracket sequence of size 10
	auto tree = e.tree(10); // a uniformly-random tree of size 10
	for (int i = 0; i < 10; ++i)
		for (int v : tree.adjacents(i))
			if (v > i) std::cout << i << " -> " << v << std::endl;
	std::cout << "===============\n";

	e.binary_tree(10);  // a uniformly-random binary tree of size 10

	std::vector<int> arr = { 1, 2, 3, 4, 5 };
	e.shuffle(arr.begin(), arr.end());
	for (int v : arr) std::cout << v << ' '; std::cout << std::endl;
}
```

### term

Basic ANSI terminal manipulation.

```cpp
#include <iostream>

#include <unistd.h>

#include <mic/term.h>

int main() {
	using namespace mic::term;

	auto error_color = bg_color(red);
	auto ok_color = bg_color(green) + fg_color(white);

	int a, b; std::cin >> a >> b;
	if (a == b) {
		std::cout << ok_color << "Two numbers are identical" << reset;
		std::cout << ": " << underline << a << no_underline << std::endl;
	} else {
		std::cout << error_color << "Two numbers differ" << reset;
		std::cout << ": " << blink << a << ' ' << b << no_blink << std::endl;
	}

	cursor::hide();
	for (int i = 1; i <= 5; ++i) {
		reset_line();
		std::cout << ok_color << '[' << i << " / 5]" << reset; std::cout.flush();
		usleep(200000);
	}
	cursor::show();
	std::cout << std::endl;

	for (int i = 0; i < 100; ++i) {
		switch (i & 3) {
			case 0: cursor::right(2); break;
			case 1: cursor::down(2); break;
			case 2: cursor::left(2); break;
			case 3: cursor::up(2); break;
		}
		std::cout << "ABCDE"[i % 5];
		cursor::left();
		std::cout.flush();
		usleep(50000);
	}
	cursor::down(3);
}
```

### zen

Data generator & checker.

Data generator:

```cpp
// Folder structure:
// - gen.cpp (this file)
// - [name].cpp (std, stdin/stdout)
#include <mic/zen.h>

using zen::map;

mic::random_engine e;
// will generate a 'data' folder automatically with testcases inside
ZEN_GEN("[name]", 20 /* amount of test cases */) {
	// arguments:
	// - out: std::ofstream to input file
	// - id: testcase id ([1..20] for this example)

	int limit;
	switch (id) {
		case 1 ... 5:
			// map from one range to another
			limit = map(id, 1, 5, 20, 100);
			break;
		default:
			limit = 500000;
			break;
	}
	out << e(0, limit) << ' ' << e(0, limit) << std::endl;
}
```

Checker:

```cpp
#include <mic/zen.h>

mic::random_engine e;
// will run forever until the outputs of two programs differ
ZEN_CHECK("a.cpp", "b.cpp") {
	out << e(0, 20000) << ' ' << e(0, 20000) << std::endl;
}
```
