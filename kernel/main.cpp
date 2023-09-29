// main.cpp
// カーネル本体がある。ブートローダからは KernelMain が呼ばれる。

#include <cstdint>
#include <cstddef>

#include "framebuffer_config.hpp"

struct PixelColor {
	uint8_t	r, g, b;
};

// 指定位置に点を描画する。
// 成功 = 0, 失敗 = 0 以外
int WritePixel(const FrameBufferConfig& conf, int x, int y, const PixelColor& c) {
	const int	pos = conf.px_per_scanline * y + x;
	uint8_t		*p;

	switch (conf.pixelformat)
	{
	case kPixelRGBResv8BitPerColor:
		p = &conf.framebuf[4 * pos];

		p[0] = c.r;
		p[1] = c.g;
		p[2] = c.b;
		break;

	case kPixelBGRResv8BitPerColor:
		p = &conf.framebuf[4 * pos];

		p[0] = c.b;
		p[1] = c.g;
		p[2] = c.r;
		break;
	
	default:
		return -1;
	}
	return 0;
}

extern "C" void KernelMain(const FrameBufferConfig& fbufconf) {
	for (int y = 0; y < fbufconf.vresolution; y++) {
		for (int x = 0; x < fbufconf.hresolution; x++) {
			WritePixel(fbufconf, x, y, { 0xff, 0xff, 0xff });
		}
	}
	for (int y = 100; y < 200; y++) {
		for (int x = 100; x < 200; x++) {
			WritePixel(fbufconf, x, y, { 0, 0xff, 0 });
		}
	}
	while (1) { __asm__("hlt"); }
}
