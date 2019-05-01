#pragma once
#include <ostream>

enum ColorCode
{
	Black = 0x0,
	Blue = 0x1,
	Green = 0x2,
	Aqua = 0x3,
	Red = 0x4,
	Purple = 0x5,
	Yellow = 0x6,
	White = 0x7,
	Gray = 0x8,
	LightBlue = 0x9,
	LightGreen = 0xA,
	LightAqua = 0xB,
	LightRed = 0xC,
	LightPurple = 0xD,
	LightYellow = 0xE,
	BrightWhite = 0xF
};

class Color
{
	static HANDLE hConsole;
	ColorCode fg_color = White, bg_color = Black;
	static ColorCode current_bg;

public:
	Color(ColorCode fg_color, ColorCode bg_color);
	Color(ColorCode fg_color);
	friend std::ostream& operator<<(std::ostream& os, Color& color);

	static void SetColor(ColorCode fg_color, ColorCode bg_color);
	static void SetColor(ColorCode fg_color);
};

