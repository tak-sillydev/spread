// main.cpp
// カーネル本体がある。ブートローダからは KernelMain が呼ばれる。

#include <cstdint>
#include <cstddef>
#include <cstdio>

#include "framebuffer_config.hpp"
#include "graphics.hpp"
#include "font.hpp"
#include "console.hpp"
#include "pci.hpp"

// <array>（pci.hpp で使用）が配置 new を呼ぶので定義廃止
// void* operator new(size_t size, void *buf) { return buf; }
void  operator delete(void* obj) noexcept {}

// デスクトップ画面を表現する色
constexpr PixelColor kDesktopBGColor{ 0x1e, 0x90, 0xff };
constexpr PixelColor kDesktopFGColor{ 0xff, 0xff, 0xff };

// マウスカーソル
constexpr int  kMouseCursor_Width  = 15;
constexpr int  kMouseCursor_Height = 24;
constexpr char mcursor_shape[kMouseCursor_Height][kMouseCursor_Width + 1] = {
	"@              ",
	"@@             ",
	"@.@            ",
	"@..@           ",
	"@...@          ",
	"@....@         ",
	"@.....@        ",
	"@......@       ",
	"@.......@      ",
	"@........@     ",
	"@.........@    ",
	"@..........@   ",
	"@...........@  ",
	"@............@ ",
	"@......@@@@@@@@",
	"@......@       ",
	"@....@@.@      ",
	"@...@ @.@      ",
	"@..@   @.@     ",
	"@.@    @.@     ",
	"@@      @.@    ",
	"@       @.@    ",
	"         @.@   ",
	"         @@@   "
};

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
	const int kFrameWidth	= fbufconf.hresolution;
	const int kFrameHeight	= fbufconf.vresolution;

	switch (fbufconf.pixelformat) {
	case kPixelRGBResv8BitPerColor:
		pxwriter = new (pxwriter_buf) RGBResv8BitPerColorPixelWriter(fbufconf);
		break;

	case kPixelBGRResv8BitPerColor:
		pxwriter = new (pxwriter_buf) BGRResv8BitPerColorPixelWriter(fbufconf);
		break;
	}
	// デスクトップ画面の描画
	FillRect(
		*pxwriter, { 0, 0 },
		{ kFrameWidth, kFrameHeight - 50 }, kDesktopBGColor
	);
	FillRect(
		*pxwriter, { 0, kFrameHeight - 50 },
		{ kFrameWidth, 50 }, { 0x59, 0x4e, 0x52 }
	);
	FillRect(
		*pxwriter, { 0, kFrameHeight - 50 },
		{ kFrameWidth / 5, 50 }, { 0xea, 0xe1, 0xcf }
	);
	DrawRect(
		*pxwriter, { 10, kFrameHeight - 40 },
		{ 30, 30 }, { 0x59, 0x4e, 0x52 }
	);

	// コンソール作成
	cons = new(cons_buf) Console(*pxwriter, kDesktopFGColor, kDesktopBGColor);
	printk("Welcome to Spread!\n");

	// マウスカーソル描画
	for (int dy = 0; dy < kMouseCursor_Height; dy++) {
		for (int dx = 0; dx < kMouseCursor_Width; dx++) {
			switch (mcursor_shape[dy][dx]) {
			case '@':
				pxwriter->Write(200 + dx, 100 + dy, { 0, 0, 0 });
				break;

			case '.':
				pxwriter->Write(200 + dx, 100 + dy, { 0xff, 0xff, 0xff });
				break;
			}
		}
	}
	// PCI バスのスキャン
	auto err = PCI::ScanAllBus();
	printk("PCI::ScanAllBus: %s\n", err.Name());

	for (int i = 0; i < PCI::ndevs; i++) {
		const auto& dev = PCI::devices[i];
		auto vendor_id = PCI::ReadVendorId(dev.bus, dev.dev, dev.func);
		auto clscode = PCI::ReadClassCode(dev.bus, dev.dev, dev.func);

		printk(
			"%02d:%02d:%d: vendor %04x, class %08x, hdrtype %02x\n",
			dev.bus, dev.dev, dev.func, vendor_id, clscode, dev.hdrtype
		);
	}
	while (1) { __asm__("hlt"); }
}
