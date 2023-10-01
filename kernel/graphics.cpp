// graphics.cpp
// 画面描画のための実装。

#include "graphics.hpp"

void RGBResv8BitPerColorPixelWriter::Write(int x, int y, const PixelColor& c) {
	auto p = PixelAt(x, y);

	p[0] = c.r;
	p[1] = c.g;
	p[2] = c.b;
}

void BGRResv8BitPerColorPixelWriter::Write(int x, int y, const PixelColor& c) {
	auto p = PixelAt(x, y);

	p[0] = c.b;
	p[1] = c.g;
	p[2] = c.r;
}

// 四角形を描画する。
void DrawRect(PixelWriter& pxwriter, const Vector2D<int>& pos,
				const Vector2D<int>& size, const PixelColor& c) {
	for (int dx = 0; dx < size.x; dx++) {
		pxwriter.Write(pos.x + dx, pos.y, c);
		pxwriter.Write(pos.x + dx, pos.y + size.y - 1, c);
	}
	for (int dy = 1; dy < size.y - 1; dy++) {
		pxwriter.Write(pos.x, pos.y + dy, c);
		pxwriter.Write(pos.x + size.x - 1, pos.y + dy, c);
	}
	return;
}

// 四角形を描画する。中も塗りつぶす。
void FillRect(PixelWriter& pxwriter, const Vector2D<int>& pos,
				const Vector2D<int>& size, const PixelColor& c) {
	for (int dy = 0; dy < size.y; dy++) {
		for (int dx = 0; dx < size.x; dx++) {
			pxwriter.Write(pos.x + dx, pos.y + dy, c);
		}
	}
	return;
}
