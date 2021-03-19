
#pragma once

#include <functional>
#include <unordered_map>
#include <unordered_set>

#include <signal.h>

namespace mic {

class SignalHandler {
public:
	static std::unordered_map<int, std::unordered_set<SignalHandler*>> signal_handlers;

	explicit SignalHandler(int signal_type, std::function<void()> func):
		signal_type(signal_type),
		func(std::move(func)) {
		static bool signal_registered = false;
		if (!signal_handlers.count(signal_type))
			signal(signal_type, static_signal_handler);
		signal_handlers[signal_type].insert(this);
	}
	SignalHandler(const SignalHandler &t) = delete;
	SignalHandler(SignalHandler &&t):
		signal_type(t.signal_type),
		func(std::move(t.func)) {}
	SignalHandler& operator=(SignalHandler&&) = default;

	~SignalHandler() { signal_handlers[signal_type].erase(this); }
private:
	static void static_signal_handler(int signal_type) {
		for (auto &handler : signal_handlers[signal_type]) handler->func();
	}

	int signal_type;
	std::function<void()> func;
};

std::unordered_map<int, std::unordered_set<SignalHandler*>> SignalHandler::signal_handlers;

} // namespace mic
