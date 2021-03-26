
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <tuple>
#include <type_traits>

#include <unistd.h>

#include "random.h"
#include "term.h"

#ifndef ZEN_COMPILER
#define ZEN_COMPILER "g++"
#endif

#ifndef ZEN_COMPILE_OPTS
#define ZEN_COMPILE_OPTS "-O2"
#endif

#define with(init) if (init; 1)
#define with_lock(mutex) with(std::unique_lock lock(mutex))

namespace zen {

using std::cerr;
using std::cout;

const auto status_color = mic::term::bg_color(mic::term::green) + mic::term::fg_color(mic::term::white);
const auto error_color = mic::term::bg_color(mic::term::red) + mic::term::fg_color(mic::term::white);
const auto subtask_color = mic::term::bg_color(mic::term::green) + mic::term::fg_color(mic::term::black);

inline int cmd(const std::string &str) { return system(str.data()); }

inline std::string read_file(const std::string &path) {
	char buf[256];
	std::string ret;
	std::ifstream in(path);
	while (size_t cnt = in.rdbuf()->sgetn(buf, sizeof(buf)))
		ret.append(buf, buf + cnt);
	return ret;
}

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
	D(compile_options, std::string, ZEN_COMPILE_OPTS)
	D(compiler, std::string, ZEN_COMPILER)
	D(config_file, ConfigFileFormat, None)
	D(data_prefix, std::string, "")
	D(input_suffix, std::string, "in")
	D(memory_limit, uint32_t, 131072) // KB
	D(output_suffix, std::string, "out")
	D(pack_type, PackType, GenOnly)
	D(parallel, bool, true)
	D(score, uint32_t, 100)
	D(score_type, ScoreType, Average)
	D(seed, uint32_t, 0x658c382b)
	D(time_limit, uint32_t, 1000) // ms
	D(UOJ_checker, std::string, "ncmp")
	D(use_subtask_directory, bool, false)
#undef D
};

class Testcase {
public:
	uint32_t id;
	uint32_t subtask_id, score;
	uint32_t time_limit; // ms
	uint32_t memory_limit; // KB

	std::ofstream *stream;

	template<class T>
	inline Testcase& operator<<(const T &t) { (*stream) << t; return *this; }

	Testcase(const Testcase &t) = delete;
	Testcase(Testcase &&t) noexcept:
		id(t.id),
		subtask_id(t.subtask_id),
		score(t.score),
		time_limit(t.time_limit),
		memory_limit(t.memory_limit),
		stream(t.stream) {}
	Testcase& operator=(Testcase &&t) noexcept = default;
private:
	Testcase(uint32_t id, uint32_t subtask_id, uint32_t score, const GenConfig &config, std::ofstream &stream):
		id(id),
		subtask_id(subtask_id),
		score(score),
		time_limit(config.time_limit),
		memory_limit(config.memory_limit),
		stream(&stream) {}

	friend class Problem;
};

using GenFuncType = std::function<void(uint32_t, Testcase&, mic::random_engine<>&&)>;

struct TestcaseGroup {
	std::string name;
	uint32_t id, num_data;
	GenFuncType gen;
};

class Problem {
public:
	explicit Problem(const std::string &name): name(name) {}
	inline bool gen();
	char reg_subtask(const std::string &name, uint32_t num_data, GenFuncType gen) {
		if (!has_subtask) {
			if (!groups.empty()) throw std::invalid_argument("You can't add subtask to a non-subtask problem");
			has_subtask = true;
		}
		groups.push_back({ name, (uint32_t)groups.size() + 1, num_data, std::move(gen) });
		return '\0';
	}
	char reg_batch(const std::string &name, uint32_t num_data, GenFuncType gen) {
		if (has_subtask) throw std::invalid_argument("You can't add non-subtask testcases to a problem that contains subtasks");
		groups.push_back({ name, (uint32_t)groups.size() + 1, num_data, std::move(gen) });
		return '\0';
	}
	void write_config_file(const std::vector<Testcase> &tests);

	GenConfig config;
private:
	std::string name;
	std::vector<TestcaseGroup> groups;
	bool has_subtask = false;
};

void Problem::write_config_file(const std::vector<Testcase> &tests) {
	switch (config.config_file) {
		case None: return;
		case Luogu: {
			std::ofstream out("data/config.yml");
			for (size_t i = 0; i < tests.size(); ++i) {
				const auto &e = tests[i];
				out << config.data_prefix << (i + 1) << "." + config.input_suffix << ":\n";
				out << "  timeLimit: " << e.time_limit << '\n';
				out << "  memoryLimit: " << e.memory_limit << '\n';
				out << "  subtaskId: " << e.subtask_id << '\n';
				out << "  score: " << e.score << '\n';
			}
			out.close();
			break;
		}
		case UOJ: {
			std::ofstream out("data/problem.conf");
			out << "use_builtin_judger on\n";
			out << "use_builtin_checker " << config.UOJ_checker << '\n';
			out << "n_tests " << tests.size() << '\n';
			out << "n_sample_tests 0\n";
			out << "n_ex_tests 0\n";
			out << "input_pre " << config.data_prefix << '\n';
			out << "input_suf " << config.input_suffix << '\n';
			out << "output_pre " << config.data_prefix << '\n';
			out << "output_suf " << config.output_suffix << '\n';
			uint32_t max_time_limit = 0, max_memory_limit = 0;
			for (const auto &test : tests) {
				max_time_limit = std::max(test.time_limit, max_time_limit);
				max_memory_limit = std::max(test.memory_limit, max_memory_limit);
			}
			out << "time_limit " << (uint32_t)std::ceil((double)max_time_limit / 1000) << '\n';
			out << "memory_limit " << (uint32_t)std::ceil((double)max_memory_limit / 256) << '\n';
			if (has_subtask) {
				out << "n_subtasks " << groups.size() << '\n';
				for (size_t i = 0; i < tests.size(); ++i) {
					if (i != tests.size() - 1 && tests[i].subtask_id == tests[i + 1].subtask_id) continue;
					out << "subtask_score_" << tests[i].subtask_id << ' ' << tests[i].score << '\n';
					out << "subtask_end_" << tests[i].subtask_id << ' ' << (i + 1) << '\n';
				}
			} else
				for (size_t i = 0; i < tests.size(); ++i)
					out << "point_score_" << (i + 1) << ' ' << tests[i].score << '\n';
			out.close();
			break;
		}
		default: assert(false);
	}
}

bool Problem::gen() {
	using namespace mic::term;
	namespace fs = std::filesystem;

	using seed_type = mic::random_engine<>::engine_type::result_type;

	if (config.use_subtask_directory) {
		if (!has_subtask) throw std::invalid_argument("You can't enable subtask directory in a non-subtask problem");
		if (config.config_file == Luogu)
			throw std::runtime_error("Subtask directory is not supported in Luogu");
		if (config.config_file == UOJ)
			throw std::runtime_error("Subtask directory is not supported in UOJ");
	}
	if (!config.checker.empty() && !fs::exists(config.checker)) {
		cerr << "Provided checker (\"" + config.checker << "\") not found\n";
		return false;
	}

	uint32_t total = 0;
	for (auto &group : groups) total += group.num_data;
	uint32_t score_thresold, score_average;
	if (config.score_type == Average) {
		const uint32_t num = has_subtask? groups.size(): total;
		score_average = 100 / num;
		score_thresold = num - (100 - num * score_average);
	}

	auto bar = std::make_unique<ProgressBar>();

	bar->set_message("Compiling std");
	if (cmd(config.compiler + " " + config.compile_options + " " + name + ".cpp -o /tmp/" + name)) {
		cerr << error_color << "Failed to compile" << (reset) << '\n';
		return false;
	}

	fs::remove_all("data");
	fs::create_directories("data");

	std::vector<Testcase> tests; tests.reserve(total);
	std::vector<std::future<void>> tasks;
	std::vector<std::function<void(seed_type)>> direct_tasks;
	std::vector<std::pair<bool, uint32_t>> subtask_score(groups.size(), { 0, 0 });
	std::vector<std::mutex> group_mutex(groups.size());
	std::vector<std::tuple<uint32_t, std::string, std::string>> errors;
	uint32_t id = 0, progress = 0;
	std::mutex finish_mutex, test_mutex;
	std::condition_variable cv;

	mic::random_engine<> rng(config.seed);
	for (auto &group : groups) {
		for (uint32_t i = 1; i <= group.num_data; ++i) {
			auto func = [&, i, id](seed_type seed) {
				std::string dir;
				if (config.use_subtask_directory) {
					dir = "data/subtask" + std::to_string(group.id);
					fs::create_directories(dir);
					dir += '/';
				} else dir = "data/";
				dir += config.data_prefix;
				auto error = [&](const std::string &e, const std::string &detail = "") {
					with_lock(finish_mutex) {
						++progress;
						errors.emplace_back(id + i, e, detail);
					}
					cv.notify_one();
				};

				const auto prefix = dir + std::to_string(id + i) + ".";
				std::ofstream stream(prefix + config.input_suffix);
				uint32_t score = -1;
				if (config.score_type == Average) {
					score = score_average;
					if ((has_subtask? group.id: (id + i)) > score_thresold) ++score;
				} else if (config.score_type == Same) score = config.score;
				Testcase test(id + i, has_subtask? group.id: 0, score, config, stream);
				try {
					group.gen(i, test, mic::random_engine<>(seed));
				} catch (const std::exception &e) {
					error("Failed to generate input", e.what());
					return;
				}
				if (config.score_type == Manual && test.score == -1) {
					error("Score type set to \"Manual\" but no score was set");
					return;
				}
				if (has_subtask)
					with_lock(group_mutex[group.id - 1]) {
						auto &score = subtask_score[group.id - 1];
						if (!score.first) score.second = test.score;
						else if (score.second != test.score) {
							error("Scores in a subtask cannot differ!");
							return;
						}
					}
				with_lock(test_mutex) tests.push_back(std::move(test));
				stream.close();
				const auto error_file = "/tmp/zen_tmp_" + name + "_" + std::to_string(id + i) + ".err";
				if (cmd("/tmp/" + name
						+ " < " + prefix + config.input_suffix
						+ " > " + prefix + config.output_suffix
						+ " 2> " + error_file)) {
					error("Failed to execute std", read_file(error_file));
					fs::remove(error_file);
					return;
				}
				fs::remove(error_file);
				with_lock(finish_mutex) ++progress;
				cv.notify_one();
			};
			if (config.parallel)
				tasks.push_back(std::async(std::launch::async, std::move(func), rng.rand<seed_type>()));
			else direct_tasks.push_back(std::move(func));
		}
		id += group.num_data;
	}

	auto show_progress = [&]() {
		const uint8_t pro_upper = config.pack_type == GenOnly? 100: 90;
		const auto pro_suffix = "/" + std::to_string(total) + ')';
		bar->set_progress(5);
		bar->set_message("Generating data (0" + pro_suffix);
		while (true) {
			std::unique_lock lock(finish_mutex); cv.wait(lock);
			if (!errors.empty()) bar->set_background_color(bg_color(color::red) + fg_color(color::white));
			bar->set_progress(std::clamp<uint8_t>(std::round((double)progress * (pro_upper - 5) / total) + 5, 5, pro_upper));
			bar->set_message("Generating data (" + std::to_string(progress) + pro_suffix);
			if (progress == total) break;
		}
	};
	if (!config.parallel) {
		// std::thread([&]() {
			// for (auto &task : direct_tasks) task(rng.rand<seed_type>());
		// }).detach();
		std::thread thr(show_progress);
		for (auto &task : direct_tasks) task(rng.rand<seed_type>());
		thr.join();
	} else {
		show_progress();
		for (auto &f : tasks) f.get();
	}
	if (!errors.empty()) {
		cerr << error_color << errors.size() << " errors occurred" << (reset) << '\n' << '\n';
		std::sort(errors.begin(), errors.end(),
					[](const decltype(errors)::value_type &x, const decltype(errors)::value_type &y) {
						return std::get<0>(x) < std::get<0>(y);
					});
		auto it = errors.begin();
		uint32_t prefix = 0;
		for (auto &group : groups) {
			using std::get;

			bool has = false;
			while (it != errors.end() && prefix < get<0>(*it) && get<0>(*it) <= (prefix + group.num_data)) {
				if (!has) {
					cerr << status_color << "=== Testcase Group: [" << group.name << "] ===" << (reset) << '\n' << '\n';
					has = true;
				}
				cerr << status_color << "Testcase " << (get<0>(*it) - prefix) << (reset)
					 << ": " << error_color << get<1>(*it) << (reset) << '\n';
				cerr << get<2>(*it) << '\n';
				++it;
			}
			prefix += group.num_data;
		}
		return false;
	}
	std::sort(tests.begin(), tests.end(),
		[](const Testcase &x, const Testcase &y) { return x.id < y.id; });
	write_config_file(tests);
	if (config.pack_type == GenOnly) { cout << '\n'; return true; }
	bar->set_progress(90);
	bar->set_message("Compressing");
	fs::remove(name + ".zip");
	if (cmd("zip -qj " + name + ".zip data/* " + config.checker)) {
		cerr << "Failed to pack\n";
		return false;
	}
	bar->set_progress(100);
	bar->set_message("Done");
	bar.reset();

	cout << "Packed to " << name << ".zip\n";
	if (config.pack_type == PackOnly) fs::remove_all("data");
	return true;
}

template<class Func, class = std::enable_if_t<std::is_invocable_r_v<void, Func, uint32_t, std::ofstream&>>>
inline bool gen(const std::string &name, uint32_t amount, const Func &func) {
	using namespace mic::term;
	namespace fs = std::filesystem;

	if (cmd(ZEN_COMPILER " " ZEN_COMPILE_OPTS " " + name + ".cpp -o /tmp/" + name)) {
		cerr << error_color << "Failed to compile" << (reset) << '\n';
		return false;
	}
	fs::remove_all("data");
	fs::create_directories("data");
	uint32_t id;
	auto info = [&](std::ostream &out = cout) -> std::ostream& {
		reset_line();
		return out << status_color << '[' << id << '/' << amount << ']' << (reset) << ' ';
	};
	for (id = 1; ; ++id) {
		const auto prefix = "data/" + std::to_string(id) + ".";
		info() << "Generating input... "; cout.flush();
		std::ofstream out(prefix + "in"); func(id, out); out.close();
		info() << "Generating output... "; cout.flush();
		if (cmd("/tmp/" + name + " < " + prefix + "in > " + prefix + "out")) {
			cerr << '\n' << error_color << "Failed to execute std" << (reset) << '\n';
			return false;
		}
		info() << "Done";
		if (id == amount) break;
	}
	cout << '\n';
	return true;
}

template<class Func, class = std::enable_if_t<std::is_invocable_r_v<void, Func, std::ofstream&>>>
inline bool check(const std::string &A, const std::string &B, const Func &gen) {
	using namespace mic::term;

	if (cmd(ZEN_COMPILER " " ZEN_COMPILE_OPTS " " + A + " -o /tmp/A")
		|| cmd(ZEN_COMPILER " " ZEN_COMPILE_OPTS " " + B + " -o /tmp/B")) {
		cerr << '\n' << error_color << "Failed to compile" << (reset) << '\n';
		return false;
	}
	uint32_t C = 0;
	auto info = [&](std::ostream &out = cout) -> std::ostream& {
		reset_line();
		return out << status_color << '[' << C << ']' << (reset) << ' ';
	};
	while (true) {
		std::ofstream out("test.in");
		++C;
		info() << "Generating... "; cout.flush();
		gen(out); out.close();

		info() << "Running A... "; cout.flush();
		if (cmd("/tmp/A < test.in > /tmp/A.out")) {
			cerr << '\n' << error_color << "Failed to execute A" << (reset) << '\n';
			return false;
		}
		info() << "Running B... "; cout.flush();
		if (cmd("/tmp/B < test.in > /tmp/B.out")) {
			cerr << '\n' << error_color << "Failed to execute B" << (reset) << '\n';
			return false;
		}
		if (cmd("diff /tmp/A.out /tmp/B.out")) {
			cerr << '\n' << error_color << "Failed" << (reset) << '\n';
			cmd("meld /tmp/A.out /tmp/B.out");
			return false;
		}
		info() << "OK";
	}
}

} // namespace zen

#undef with_lock

#define PROBLEM(name) \
	namespace zen { Problem main(#name); } \
	int main() { zen::main.gen(); }

#define SUBTASK(name, num) \
	namespace zen { \
	void subtask_##name##_impl(uint32_t id, Testcase &out, mic::random_engine<> &&e); \
	const char subtask_##name##_helper = main.reg_subtask(#name, num, subtask_##name##_impl); \
	} \
	void zen::subtask_##name##_impl(uint32_t id, Testcase &out, mic::random_engine<> &&e)

#define BATCH(name, num) \
	namespace zen { \
	void subtask_##name##_impl(uint32_t id, Testcase &out, mic::random_engine<> &&e); \
	const char subtask_##name##_helper = main.reg_batch(#name, num, subtask_##name##_impl); \
	} \
	void zen::subtask_##name##_impl(uint32_t id, Testcase &out, mic::random_engine<> &&e)

#define CONFIG( ... ) \
	namespace zen { \
		struct config_static_code_helper { \
			config_static_code_helper() { zen::main.config = __VA_ARGS__; } \
		} _config_static_code_helper_instance; \
	}

#define ZEN_GEN(name, amount) \
	inline void gen(uint32_t id, std::ofstream &out); \
	int main() { zen::gen(name, amount, gen); } \
	inline void gen(uint32_t id, std::ofstream &out)

#define ZEN_CHECK(A, B) \
	inline void gen(std::ofstream &out); \
	int main() { zen::check(A, B, gen); } \
	inline void gen(std::ofstream &out)
