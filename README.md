
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
