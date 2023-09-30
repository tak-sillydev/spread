#pragma once

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

	virtual void Write(int x, int y, const PixelColor& c) override;
};

class BGRResv8BitPerColorPixelWriter : public PixelWriter {
public:
	using PixelWriter::PixelWriter;

	virtual void Write(int x, int y, const PixelColor& c) override;
};