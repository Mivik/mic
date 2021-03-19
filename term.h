
#pragma once

#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>

#include <sys/ioctl.h>
#include <unistd.h>

#include "signal.h"

namespace mic::term {
#define F(name, str) inline std::ostream& name(std::ostream &out) { return out << str; }
#define D(name, str) inline void name() { std::cout << str; }

inline void flush() { std::cout.flush(); }

F(reset, "\e[;0m")
F(underline, "\e[;4m")
F(no_underline, "\e[;24m")
F(blink, "\e[;5m")
F(no_blink, "\e[;25m")

enum color {
	undef = -1,
	black = 0, red = 1, green = 2, yellow = 3,
	blue = 4, magenta = 5, cyan = 6, grey = 7,
	white = 100,
};

struct color_manip {
	color fg, bg;
	bool bright;
	color_manip(): fg(undef), bg(undef), bright(false) {}
	color_manip(color fg, color bg, bool bright):
		fg(fg), bg(bg), bright(bright) {}
	inline color_manip operator+(const color_manip &other) const {
		color_manip ret = *this;
		if (other.fg != undef) { ret.fg = other.fg; ret.bright = other.bright; }
		if (other.bg != undef) ret.bg = other.bg;
		return ret;
	}
	inline bool operator==(const color_manip &other) const {
		return fg == other.fg && bg == other.bg && bright == other.bright;
	}
	friend inline std::ostream& operator<<(std::ostream &out, const color_manip &c) {
		out << "\e[";
		if (c.fg == undef) out << '0';
		else if (c.bright) {
			assert(c.fg != black && c.fg != grey && c.fg != white && "bright black, grey and white are non-sense");
			out << "1;" << (40 + c.fg);
		} else {
			if (c.fg == white) out << "1;37";
			else out << "0;" << (30 + c.fg);
		}
		if (c.bg != undef) {
			out << ';';
			out << (40 + c.bg);
		}
		return out << 'm';
	}
};

inline color_manip fg_color(color c, bool bright = false) {
	return color_manip(c, undef, bright);
}

inline color_manip bg_color(color c) {
	assert(c != white && "background color cannot be white");
	return color_manip(undef, c, false);
}

D(home, "\e[H")
D(clear, "\e[2J")
D(clear_line_before_cursor, "\e[1K")
D(clear_line, "\e[2K")
D(begin_of_line, "\r")
inline void reset_line() {
	clear_line();
	begin_of_line();
}

namespace cursor {

D(hide, "\e[?25l")
D(show, "\e[?25h")
D(save, "\e7")
D(restore, "\e8")

inline void up(int amount = 1) { std::cout << "\e[" << amount << 'A'; flush(); }
inline void down(int amount = 1) { std::cout << "\e[" << amount << 'B'; flush(); }
inline void right(int amount = 1) { std::cout << "\e[" << amount << 'C'; flush(); }
inline void left(int amount = 1) { std::cout << "\e[" << amount << 'D'; flush(); }

inline void move(int row, int col) {
	assert(row >= 0 && col >= 0);
	std::cout << "\e[" << (row + 1) << ';' << (col + 1) << 'H';
	flush();
}

} // namespace cursor

struct WindowSize {
public:
	static WindowSize get() {
		static winsize size;
		ioctl(STDIN_FILENO, TIOCGWINSZ, (char*)&size);
		return WindowSize(size.ws_col, size.ws_row);
	}

	uint32_t width, height;
private:
	WindowSize(uint32_t width, uint32_t height): width(width), height(height) {}
};

class WindowResizeListener : public SignalHandler {
public:
	explicit WindowResizeListener(std::function<void(const WindowSize&)> func):
		SignalHandler(SIGWINCH, [ func{std::move((func(WindowSize::get()), func))} ](){
			func(WindowSize::get());
		}) {}
	WindowResizeListener(const WindowResizeListener &t) = delete;
	WindowResizeListener(WindowResizeListener &&t): SignalHandler(std::move(t)) {}
	WindowResizeListener& operator=(WindowResizeListener &&t) = default;
};

class ProgressBar {
public:
	static const term::color_manip status_color;

	ProgressBar():
		message(""),
		progress(0),
		background_color(term::bg_color(grey) + term::fg_color(black)),
		listener([this](const WindowSize &size) { draw(size); }) {}
	ProgressBar(const ProgressBar &t) = delete;
	ProgressBar(ProgressBar &&t):
		message(std::move(t.message)),
		progress(t.progress),
		background_color(t.background_color),
		listener(std::move(t.listener)) {}

	void draw(const WindowSize &size = WindowSize::get()) {
		using term::reset;

		assert(size.width >= 12 && "The width of the window is too small to display a progress bar");
		begin_of_line();

		std::cout << status_color << '[' << std::setw(3) << std::setfill(' ') << (int)progress << "%]" << (reset);
		std::cout << ' ';
		uint32_t rem = size.width - 7, num = std::ceil((double)progress * rem / 100), str_maxlen = rem - 2;
		std::string display = message.size() > str_maxlen? (message.substr(0, str_maxlen - 3) + "..."): message;
		uint32_t begin = (rem - display.size() + 1) / 2, end = begin + display.size();
		assert(end <= rem);
		std::cout << background_color;
		for (uint32_t i = 0; i < rem; ++i) {
			if (i == num) std::cout << (reset);
			if (i < begin) std::cout << ' ';
			else if (i < end) std::cout << display[i - begin];
			else std::cout << ' ';
		}
		std::cout << (reset);
		std::cout.flush();
	}

	inline void set_progress(uint8_t progress) {
		assert(0 <= progress && progress <= 100);
		if (this->progress == progress) return;
		this->progress = progress; draw();
	}
	[[nodiscard]] inline uint8_t get_progress() const { return progress; }
	inline void set_message(const std::string &message) {
		if (this->message == message) return;
		this->message = message; draw();
	}
	[[nodiscard]] inline const std::string& get_message() const { return message; }
	inline void set_background_color(const term::color_manip &color) {
		if (background_color == color) return;
		this->background_color = color; draw();
	}
	[[nodiscard]] inline const term::color_manip& get_background_color() const { return background_color; }
private:
	std::string message;
	uint8_t progress;
	term::color_manip background_color;
	WindowResizeListener listener;
};

const term::color_manip
	ProgressBar::status_color = term::bg_color(green) + term::fg_color(white);

#undef F
#undef D
} // namespace mic::term
