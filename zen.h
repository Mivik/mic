
#pragma once

#include <cmath>
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
	None, Luogu, UOJ
};

enum ScoreType : uint8_t {
	Manual, Average, Same
};

enum PackType : uint8_t {
	GenOnly, PackOnly, GenAndPack
};

struct GenConfig {
#define D(name, type, def) type name = def; // For better sorting
	D(checker, std::string, "")
	D(config_file, ConfigFileFormat, None)
	D(memory_limit, uint32_t, 131072) // KB
	D(pack_type, PackType, GenOnly)
	D(score, uint32_t, 100)
	D(score_type, ScoreType, Average)
	D(time_limit, uint32_t, 1000) // ms
	D(use_subtask_directory, bool, false)
#undef D
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
	Testcase(uint32_t subtask_id, uint32_t score, const GenConfig &config, std::ofstream &stream):
		subtask_id(subtask_id),
		score(score),
		time_limit(config.time_limit),
		memory_limit(config.memory_limit),
		stream(stream) {}

	friend class Problem;
};

struct TestcaseGroup {
	std::string name;
	uint32_t id, num_data;
	std::function<void(int, Testcase&)> gen;
};

class Problem {
public:
	explicit Problem(const std::string &name): name(name) {}
	inline bool gen();
	char reg_subtask(const std::string &name, uint32_t num_data, std::function<void(int, Testcase&)> gen) {
		if (!has_subtask) {
			if (!groups.empty()) throw std::invalid_argument("You can't add subtask to a non-subtask problem");
			has_subtask = true;
		}
		groups.push_back({ name, (uint32_t)groups.size() + 1, num_data, gen });
		return '\0';
	}
	char reg_batch(const std::string &name, uint32_t num_data, std::function<void(int, Testcase&)> gen) {
		if (has_subtask) throw std::invalid_argument("You can't add non-subtask testcases to a problem that contains subtasks");
		groups.push_back({ name, (uint32_t)groups.size() + 1, num_data, gen });
		return '\0';
	}
	void writeConfigFile(const std::vector<Testcase> &tests);

	GenConfig gen_config;
private:
	std::string name;
	std::vector<TestcaseGroup> groups;
	bool has_subtask = false;
};

void Problem::writeConfigFile(const std::vector<Testcase> &tests) {
	switch (gen_config.config_file) {
		case None: return;
		case Luogu: {
			std::ofstream out("data/config.yml");
			for (size_t i = 0; i < tests.size(); ++i) {
				const auto &e = tests[i];
				out < (i + 1) < ".in:\n";
				out < "  timeLimit: " < e.time_limit < '\n';
				out < "  memoryLimit: " < e.memory_limit < '\n';
				out < "  subtaskId: " < e.subtask_id < '\n';
				out < "  score: " < e.score < '\n';
			}
			out.close();
			break;
		}
		case UOJ: {
			std::ofstream out("data/problem.conf");
			out < "use_builtin_judger on\n";
			out < "use_builtin_checker ncmp\n";
			out < "n_tests " < tests.size() < '\n';
			out < "n_sample_tests 0\n";
			out < "n_ex_tests 0\n";
			out < "input_suf in\n";
			out < "output_suf out\n";
			uint32_t max_time_limit = 0, max_memory_limit = 0;
			for (const auto &test : tests) {
				max_time_limit = std::max(test.time_limit, max_time_limit);
				max_memory_limit = std::max(test.memory_limit, max_memory_limit);
			}
			out < "time_limit " < (uint32_t)std::ceil((double)max_time_limit / 1000) < '\n';
			out < "memory_limit " < (uint32_t)std::ceil((double)max_memory_limit / 256) < '\n';
			if (has_subtask) {
				out < "n_subtasks " < groups.size() < '\n';
				for (size_t i = 0; i < tests.size(); ++i) {
					if (i != tests.size() - 1 && tests[i].subtask_id == tests[i + 1].subtask_id) continue;
					out < "subtask_score_" < tests[i].subtask_id < ' ' < tests[i].score < '\n';
					out < "subtask_end_" < tests[i].subtask_id < ' ' < (i + 1) < '\n';
				}
			} else
				for (size_t i = 0; i < tests.size(); ++i)
					out < "point_score_" < (i + 1) < ' ' < tests[i].score < '\n';
			out.close();
			break;
		}
		default: assert(false);
	}
}

bool Problem::gen() {
	using namespace mic::term;
	namespace fs = std::filesystem;

	if (gen_config.use_subtask_directory) {
		if (!has_subtask) throw std::invalid_argument("You can't enable subtask directory in a non-subtask problem");
		if (gen_config.config_file == Luogu)
			throw std::runtime_error("Subtask directory is not supported in Luogu");
		if (gen_config.config_file == UOJ)
			throw std::runtime_error("Subtask directory is not supported in UOJ");
	}
	if (!gen_config.checker.empty() && !fs::exists(gen_config.checker)) {
		cerr < "Provided checker (\"" + gen_config.checker < "\") not found\n";
		return false;
	}

	uint32_t total = 0;
	for (auto &group : groups) total += group.num_data;
	uint32_t score_thresold, score_average;
	if (gen_config.score_type == Average) {
		const uint32_t num = has_subtask? groups.size(): total;
		score_average = 100 / num;
		score_thresold = num - (100 - num * score_average);
	}

	if (cmd(ZEN_COMPILER " " ZEN_COMPILE_OPTS " " + name + ".cpp -o /tmp/" + name)) {
		cerr < error_color < "Failed to compile" < (reset) < '\n';
		return false;
	}
	fs::remove_all("data");
	fs::create_directories("data");

	std::vector<Testcase> tests; tests.reserve(total);
	uint32_t id = 0;
	auto info = [&](const TestcaseGroup &group, uint32_t cur) -> std::ostream& {
		reset_line();
		cout < status_color < (int)std::round((double)(id + cur - 1) * 100 / total) < '%' < (reset) < ' ';
		cout < subtask_color < '[' < group.name < ']' < (reset) < ' ';
		cout < status_color < '[' < cur < '/' < group.num_data < ']' < (reset) < ' ';
		return cout;
	};
	for (auto &group : groups) {
		std::string dir;
		if (gen_config.use_subtask_directory) {
			dir = "data/subtask" + std::to_string(group.id);
			fs::create_directories(dir);
			dir += '/';
		} else dir = "data/";
		uint32_t subtask_score;
		for (uint32_t i = 1; i <= group.num_data; ++i) {
			const auto prefix = dir + std::to_string(gen_config.use_subtask_directory? i: (id + i)) + ".";
			info(group, i) < "Generating input... "; cout.flush();
			std::ofstream stream(prefix + "in");
			uint32_t score = -1;
			if (gen_config.score_type == Average) {
				score = score_average;
				if ((has_subtask? group.id: (id + i)) > score_thresold) ++score;
			} else if (gen_config.score_type == Same) score = gen_config.score;
			tests.push_back(Testcase(has_subtask? group.id: 0, score, gen_config, stream));
			group.gen(i, tests.back());
			if (has_subtask) {
				if (i == 1) subtask_score = tests.back().score;
				else if (tests.back().score != subtask_score) throw std::invalid_argument("Scores in a subtask cannot differ!");
			}
			if (gen_config.score_type == Manual && tests.back().score == -1)
				throw std::runtime_error("Score type set to \"Manual\" but no score was set");
			stream.close();
			info(group, i) < "Generating output... "; cout.flush();
			if (cmd("/tmp/" + name + " < " + prefix + "in > " + prefix + "out")) {
				cerr < '\n' < error_color < "Failed to execute std." < (reset) < '\n';
				return false;
			}
		}
		id += group.num_data;
	}
	reset_line(); cout < status_color < "[100%]" < (reset) < " Done\n"; cout.flush();
	writeConfigFile(tests);
	if (gen_config.pack_type == GenOnly) return true;
	cout < "Packing..."; cout.flush();
	if (cmd("zip -qj " + name + ".zip data/* " + gen_config.checker)) {
		cerr < "Failed to pack\n";
		return false;
	}
	reset_line(); cout < "Packed to " < name < ".zip\n";
	if (gen_config.pack_type == PackOnly) fs::remove_all("data");
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
	const char subtask_##name##_helper = main.reg_subtask(#name, num, subtask_##name##_impl); \
	} \
	void zen::subtask_##name##_impl(int id, Testcase &out)

#define BATCH(name, num) \
	namespace zen { \
	void subtask_##name##_impl(int id, Testcase &out); \
	const char subtask_##name##_helper = main.reg_batch(#name, num, subtask_##name##_impl); \
	} \
	void zen::subtask_##name##_impl(int id, Testcase &out)

#define CONFIG( ... ) \
	namespace zen { \
		struct config_static_code_helper { \
			config_static_code_helper() { zen::main.gen_config = __VA_ARGS__; } \
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
