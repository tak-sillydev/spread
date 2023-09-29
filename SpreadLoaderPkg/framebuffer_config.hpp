#pragma once

#include <stdint.h>

enum PixelFormat {
	kPixelRGBResv8BitPerColor,
	kPixelBGRResv8BitPerColor,
};

typedef struct {
	uint8_t		*framebuf;
	uint32_t	px_per_scanline;
	uint32_t	hresolution;
	uint32_t	vresolution;
	enum PixelFormat	pixelformat;
} FrameBufferConfig;
