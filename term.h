
#pragma once

#include <iostream>
#include <cassert>

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
	inline color_manip operator+(const color_manip &other) {
		color_manip ret = *this;
		if (other.fg != undef) { ret.fg = other.fg; ret.bright = other.bright; }
		if (other.bg != undef) ret.bg = other.bg;
		return ret;
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

#undef F
#undef D
} // namespace mic::term
