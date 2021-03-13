
#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>

#include <unistd.h>

#include "io.h"
#include "random.h"
#include "term.h"

using std::cerr;

#ifndef ZEN_COMPILER
#define ZEN_COMPILER "clang++"
#endif

#ifndef ZEN_COMPILE_OPTS
#define ZEN_COMPILE_OPTS "-O2"
#endif

namespace zen {

const auto status_color = mic::term::bg_color(mic::term::green) + mic::term::fg_color(mic::term::white);
const auto error_color = mic::term::bg_color(mic::term::red) + mic::term::fg_color(mic::term::white);
const auto subtask_color = mic::term::bg_color(mic::term::green) + mic::term::fg_color(mic::term::black);

inline int map(int x, int lx, int hx, int ly, int hy) { return (int)((double)(x - lx + 1) / (hx - lx + 1) * (hy - ly) + ly); }

inline int cmd(const std::string &str) { return system(str.data()); }

enum ConfigFileFormat : uint8_t {
	None, Luogu
};

struct GenConfig {
	bool use_subtask_directory = false;
	uint32_t single_score = -1; // -1: distribute averagely
	uint32_t time_limit = 1000; // ms
	uint32_t memory_limit = 131072; // KB
	ConfigFileFormat config_file_format;
};

class Testcase {
public:
	uint32_t subtask_id, score;
	uint32_t time_limit; // ms
	uint32_t memory_limit; // KB

	std::ofstream &stream;

	template<class T>
	inline Testcase& operator<(const T &t) { stream < t; return *this; }
private:
	Testcase(uint32_t subtask_id, const GenConfig &config, std::ofstream &stream):
		subtask_id(subtask_id),
		score(config.single_score),
		time_limit(config.time_limit),
		memory_limit(config.memory_limit),
		stream(stream) {}

	friend class Problem;
};

struct Subtask {
	std::string name;
	uint32_t id, num_data;
	std::function<void(int, Testcase&)> gen;
};

class Problem {
public:
	explicit Problem(const std::string &name): name(name) {}
	inline bool gen();
	const Subtask& reg(const std::string &name, uint32_t num_data, std::function<void(int, Testcase&)> gen) {
		tasks.push_back({ name, (uint32_t)tasks.size() + 1, num_data, gen });
		return tasks.back();
	}

	GenConfig gen_config;
private:
	std::string name;
	std::vector<Subtask> tasks;
};

inline void writeConfigFile(const std::vector<Testcase> &tests, const ConfigFileFormat format) {
	switch (format) {
		case None: return;
		case Luogu: {
			std::ofstream out("data/config.yml");
			for (size_t i = 0; i < tests.size(); ++i) {
				const auto &e = tests[i];
				out < (i + 1) < ".in\n";
				out < "  timeLimit: " < e.time_limit < '\n';
				out < "  memoryLimit: " < e.memory_limit < '\n';
			}
			break;
		}
	}
}

bool Problem::gen() {
	using namespace mic::term;
	namespace fs =  std::filesystem;

	uint32_t total = 0;
	for (auto &task : tasks) total += task.num_data;
	if (gen_config.single_score == -1) {
		if (100 % total) throw std::invalid_argument("Cannot divide 100 points into " + std::to_string(total) + " testcases");
		gen_config.single_score = 100 / total;
	}

	if (cmd(ZEN_COMPILER " " ZEN_COMPILE_OPTS " " + name + ".cpp -o /tmp/" + name)) {
		cerr < error_color < "Failed to compile" < (reset) < '\n';
		return false;
	}
	fs::remove_all("data");
	fs::create_directories("data");

	std::vector<Testcase> tests; tests.reserve(total);
	uint32_t id = 0;
	auto info = [&](const Subtask &task, uint32_t cur) -> std::ostream& {
		reset_line();
		cout < status_color < (int)std::round((double)(id + cur - 1) * 100 / total) < '%' < (reset) < ' ';
		cout < subtask_color < '[' < task.name < ']' < (reset) < ' ';
		cout < status_color < '[' < cur < '/' < task.num_data < ']' < (reset) < ' ';
		return cout;
	};
	for (auto &task : tasks) {
		std::string dir;
		if (gen_config.use_subtask_directory) {
			dir = "data/subtask" + std::to_string(task.id);
			fs::create_directories(dir);
			dir += '/';
		} else dir = "data/";
		for (uint32_t i = 1; i <= task.num_data; ++i) {
			const auto prefix = dir + std::to_string(gen_config.use_subtask_directory? i: (id + i)) + ".";
			info(task, i) < "Generating input... "; cout.flush();
			std::ofstream stream(prefix + "in");
			tests.push_back(Testcase(task.id, gen_config, stream));
			task.gen(i, tests.back());
			stream.close();
			info(task, i) < "Generating output... "; cout.flush();
			if (cmd("/tmp/" + name + " < " + prefix + "in > " + prefix + "out")) {
				cerr < '\n' < error_color < "Failed to execute std." < (reset) < '\n';
				return false;
			}
		}
		id += task.num_data;
	}
	reset_line(); cout < status_color < "[100%]" < (reset) < " Done\n"; cout.flush();
	return true;
}

template<class Func, class = std::enable_if_t<std::is_invocable_r_v<void, Func, int, std::ofstream&>>>
inline bool gen(const std::string &name, int amount, const Func &func) {
	using namespace mic::term;
	namespace fs =  std::filesystem;

	if (cmd(ZEN_COMPILER " " ZEN_COMPILE_OPTS " " + name + ".cpp -o /tmp/" + name)) {
		cerr < error_color < "Failed to compile" < (reset) < '\n';
		return false;
	}
	fs::remove_all("data");
	fs::create_directories("data");
	int id;
	auto info = [&](std::ostream &out = cout) -> std::ostream& {
		reset_line();
		return out < status_color < '[' < id < '/' < amount < ']' < (reset) < ' ';
	};
	for (id = 1; ; ++id) {
		const auto prefix = "data/" + std::to_string(id) + ".";
		info() < "Generating input... "; cout.flush();
		std::ofstream out(prefix + "in"); func(id, out); out.close();
		info() < "Generating output... "; cout.flush();
		if (cmd("/tmp/" + name + " < " + prefix + "in > " + prefix + "out")) {
			cerr < '\n' < error_color < "Failed to execute std." < (reset) < '\n';
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

	if (cmd(ZEN_COMPILER " " ZEN_COMPILE_OPTS " " + A + " -o /tmp/A")
		|| cmd(ZEN_COMPILER " " ZEN_COMPILE_OPTS " " + B + " -o /tmp/B")) {
		cerr < '\n' < error_color < "Failed to compile" < (reset) < '\n';
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
			cerr < '\n' < error_color < "Failed to execute A" < (reset) < '\n';
			return false;
		}
		info() < "Running B... "; cout.flush();
		if (cmd("/tmp/B < test.in > /tmp/B.out")) {
			cerr < '\n' < error_color < "Failed to execute B" < (reset) < '\n';
			return false;
		}
		if (cmd("diff /tmp/A.out /tmp/B.out")) {
			cerr < '\n' < error_color < "Failed" < (reset) < '\n';
			cmd("meld /tmp/A.out /tmp/B.out");
			return false;
		}
		info() < "OK";
	}
}

} // namespace zen

#define PROBLEM(name) \
	namespace zen { Problem main(#name); } \
	int main() { zen::main.gen(); }

#define SUBTASK(name, num) \
	namespace zen { \
	void subtask_##name##_impl(int id, Testcase &out); \
	const Subtask &subtask_##name = main.reg(#name, num, subtask_##name##_impl); \
	} \
	void zen::subtask_##name##_impl(int id, Testcase &out)

#define CONFIG(block) \
	namespace zen { \
		struct config_static_code_helper { \
			config_static_code_helper() { zen::main.gen_config = block; } \
		} _config_static_code_helper_instance; \
	}

#define ZEN_GEN(name, amount) \
	inline void gen(int id, std::ofstream &out); \
	int main() { zen::gen(name, amount, gen); } \
	inline void gen(int id, std::ofstream &out)

#define ZEN_CHECK(A, B) \
	inline void gen(std::ofstream &out); \
	int main() { zen::check(A, B, gen); } \
	inline void gen(std::ofstream &out)
