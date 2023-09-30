// main.cpp
// カーネル本体がある。ブートローダからは KernelMain が呼ばれる。

#include <cstdint>
#include <cstddef>

#include "framebuffer_config.hpp"
#include "graphics.hpp"
#include "font.hpp"

void* operator new(size_t size, void *buf) { return buf; }
void  operator delete(void* obj) noexcept {}

char pxwriter_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter *pxwriter;

extern "C" void KernelMain(const FrameBufferConfig& fbufconf) {
	switch (fbufconf.pixelformat) {
	case kPixelRGBResv8BitPerColor:
		pxwriter = new (pxwriter_buf) RGBResv8BitPerColorPixelWriter(fbufconf);
		break;

	case kPixelBGRResv8BitPerColor:
		pxwriter = new (pxwriter_buf) BGRResv8BitPerColorPixelWriter(fbufconf);
		break;
	}
	for (int y = 0; y < fbufconf.vresolution; y++) {
		for (int x = 0; x < fbufconf.hresolution; x++) {
			pxwriter->Write(x, y, { 0xff, 0xff, 0xff });
		}
	}
	for (int y = 100; y < 200; y++) {
		for (int x = 100; x < 200; x++) {
			pxwriter->Write(x, y, { 0, 0xff, 0 });
		}
	}
	WriteAscii(*pxwriter, 50, 50, 'A', { 0, 0, 0 });
	WriteAscii(*pxwriter, 58, 50, 'A', { 0, 0, 0 });

	while (1) { __asm__("hlt"); }
}
