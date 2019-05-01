#include "pch.h"
#include "Color.h"

ColorCode Color::current_bg = Black;
HANDLE Color::hConsole = nullptr;

Color::Color(const ColorCode fg_color, const ColorCode bg_color) : fg_color(fg_color), bg_color(bg_color) {}

Color::Color(const ColorCode fg_color) : fg_color(fg_color) {}

void Color::SetColor(const ColorCode fg_color, const ColorCode bg_color)
{
	current_bg = bg_color;
	hConsole = hConsole == nullptr ? GetStdHandle(STD_OUTPUT_HANDLE) : hConsole;
	SetConsoleTextAttribute(hConsole, static_cast<int>(fg_color) + static_cast<int>(bg_color) * 16);
}

void Color::SetColor(const ColorCode fg_color)
{
	hConsole = hConsole == nullptr ? GetStdHandle(STD_OUTPUT_HANDLE) : hConsole;
	SetConsoleTextAttribute(hConsole, static_cast<int>(fg_color) + static_cast<int>(Black) * 16);
}

std::ostream& operator <<(std::ostream& os, Color& color)
{
	Color::SetColor(color.fg_color, color.bg_color);
	return os;
}
