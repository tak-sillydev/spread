#pragma once

#include <cstdint>
#include "graphics.hpp"

void WriteAscii(PixelWriter& pxwriter, int x, int y, char ch, const PixelColor& c);
void WriteString(PixelWriter& pxwriter, int x, int y, const char *s, const PixelColor& c);
