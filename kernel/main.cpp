// main.cpp
// カーネル本体がある。ブートローダからは KernelMain が呼ばれる。

#include <cstdint>
#include <cstddef>

#include "framebuffer_config.hpp"

struct PixelColor {
	uint8_t	r, g, b;
};

class PixelWriter {
public:
	PixelWriter(const FrameBufferConfig& conf) : config_(conf) {}
	virtual ~PixelWriter() = default;

	virtual void Write(int x, int y, const PixelColor& c) = 0;

protected:
	uint8_t* PixelAt(int x, int y) {
		return config_.framebuf + 4 * (config_.px_per_scanline * y + x); 
	}

private:
	const FrameBufferConfig& config_;
};

class RGBResv8BitPerColorPixelWriter : public PixelWriter {
public:
	using PixelWriter::PixelWriter;	// 子クラスのコンストラクタが親のものに転送される

	virtual void Write(int x, int y, const PixelColor& c) override {
		auto p = PixelAt(x, y);

		p[0] = c.r;
		p[1] = c.g;
		p[2] = c.b;
	}
};

class BGRResv8BitPerColorPixelWriter : public PixelWriter {
public:
	using PixelWriter::PixelWriter;

	virtual void Write(int x, int y, const PixelColor& c) override {
		auto p = PixelAt(x, y);

		p[0] = c.b;
		p[1] = c.g;
		p[2] = c.r;
	}
};

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
	while (1) { __asm__("hlt"); }
}
