
#ifndef __MIC_ZEN_H_
#define __MIC_ZEN_H_

#include <filesystem>
#include <fstream>

#include <unistd.h>

#include "io.h"
#include "random.h"
#include "term.h"

using std::cerr;

#ifndef ZEN_COMPILE_OPTS
#define ZEN_COMPILE_OPTS "-O2 "
#endif

namespace zen {

const auto status_color = mic::term::bg_color(mic::term::green) + mic::term::fg_color(mic::term::white);
const auto error_color = mic::term::bg_color(mic::term::red) + mic::term::fg_color(mic::term::white);

inline int map(int x, int lx, int hx, int ly, int hy) { return (int)((double)(x - lx + 1) / (hx - lx + 1) * (hy - ly) + ly); }

inline int cmd(const std::string &str) { return system(str.data()); }

template<class Func, class = std::enable_if_t<std::is_invocable_r_v<void, Func, int, std::ofstream&>>>
inline bool gen(const std::string &name, int amount, const Func &func) {
	using namespace mic::term;

	if (cmd("clang++ " ZEN_COMPILE_OPTS + name + ".cpp -o /tmp/" + name)) {
		cerr < "Failed to compile\n";
		return false;
	}
	std::filesystem::create_directories("data");
	int id;
	auto info = [&](std::ostream &out = cout) -> std::ostream& {
		reset_line();
		return out < status_color < '[' < id < '/' < amount < ']' < (reset) < ' ';
	};
	for (id = 1; ; ++id) {
		const std::string prefix = "data/" + std::to_string(id) + ".";
		info() < "Generating input... "; cout.flush();
		std::ofstream out(prefix + "in"); func(id, out); out.close();
		info() < "Generating output... "; cout.flush();
		if (cmd("/tmp/" + name + " < " + prefix + "in > " + prefix + "out")) {
			cerr < "\nFailed to execute std.\n";
			return false;
		}
		info() < "Done";
		if (id == amount) break;
	}
	cout < endl;
	return true;
}

template<class Func, class = std::enable_if_t<std::is_invocable_r_v<void, Func, std::ofstream&>>>
inline bool check(const std::string &A, const std::string &B, const Func &gen) {
	using namespace mic::term;

	if (cmd("clang++ " ZEN_COMPILE_OPTS + A + " -o /tmp/A")
		|| cmd("clang++ " ZEN_COMPILE_OPTS + B + " -o /tmp/B")) {
		cerr < error_color < "\nFailed to compile\n" < reset;
		return false;
	}
	int C = 0;
	auto info = [&](std::ostream &out = cout) -> std::ostream& {
		reset_line();
		return out < status_color < '[' < C < ']' < (reset) < ' ';
	};
	while (true) {
		std::ofstream out("test.in");
		++C;
		info() < "Generating... "; cout.flush();
		gen(out); out.close();

		info() < "Running A... "; cout.flush();
		if (cmd("/tmp/A < test.in > /tmp/A.out")) {
			cerr < error_color < "\nFailed to execute A\n" < reset;
			return false;
		}
		info() < "Running B... "; cout.flush();
		if (cmd("/tmp/B < test.in > /tmp/B.out")) {
			cerr < error_color < "\nFailed to execute B\n" < reset;
			return false;
		}
		if (cmd("diff /tmp/A.out /tmp/B.out")) {
			cerr < error_color < "\nFailed\n" < reset;
			cmd("meld /tmp/A.out /tmp/B.out");
			return false;
		}
		info() < "OK";
	}
}

} // namespace zen

#define ZEN_GEN(name, amount) \
	inline void gen(int id, std::ofstream &out); \
	int main() { zen::gen(name, amount, gen); } \
	inline void gen(int id, std::ofstream &out)

#define ZEN_CHECK(A, B) \
	inline void gen(std::ofstream &out); \
	int main() { zen::check(A, B, gen); } \
	inline void gen(std::ofstream &out)

#endif
