// main.cpp
// カーネル本体がある。ブートローダからは KernelMain が呼ばれる。

#include <cstdint>
#include <cstddef>
#include <cstdio>

#include "framebuffer_config.hpp"
#include "graphics.hpp"
#include "font.hpp"
#include "console.hpp"

void* operator new(size_t size, void *buf) { return buf; }
void  operator delete(void* obj) noexcept {}

char pxwriter_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter *pxwriter;

char cons_buf[sizeof(Console)];
Console *cons;

// カーネル メッセージの出力。
// 出力された（はずの）文字数を返す。
int printk(const char *format, ...) {
	va_list	ap;
	char	s[1024];
	int		result;

	va_start(ap, format);
	result = vsnprintf(s, sizeof(s) / sizeof(char), format, ap);
	va_end(ap);

	cons->PutString(s);
	return result;
}

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
	cons = new(cons_buf) Console(*pxwriter, { 0, 0, 0 }, { 0xff, 0xff, 0xff });

	for (int i = 0; i < Console::kRows + 2; i++) {
		printk("printk line: %d\n", i);
	}
	while (1) { __asm__("hlt"); }
}
set