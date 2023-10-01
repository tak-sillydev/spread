#pragma once

#include <cstdint>
#include "graphics.hpp"

#define	FONT_WIDTH			8
#define	FONT_HEIGHT			16
#define	FONT_BINARY_SIZE	16

void WriteAscii(PixelWriter& pxwriter, int x, int y, char ch, const PixelColor& c);
void WriteString(PixelWriter& pxwriter, int x, int y, const char *s, const PixelColor& c);
