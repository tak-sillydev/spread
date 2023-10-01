// console.cpp
// コンソール クラスの定義。カーネル メッセージの出力に用いる
#include "console.hpp"

#include <cstring>
#include "font.hpp"

Console::Console(PixelWriter& pxwriter, const PixelColor& c_fg, const PixelColor& c_bg) :
	pxwriter_(pxwriter), c_fg_(c_fg), c_bg_(c_bg), buf_(), cur_row_(0), cur_col_(0) {}

void Console::PutString(const char *s) {
	while (*s) {
		if (*s == '\n') {
			NewLine();
		}
		else if (cur_col_ < kCols - 1) {
			WriteAscii(pxwriter_, cur_col_ * 8, cur_row_ * 16, *s, c_fg_);
			buf_[cur_row_][cur_col_] = *s;
			cur_col_++;
		}
		s++;
	}
	return;
}

void Console::NewLine() {
	cur_col_ = 0;

	if (cur_row_ < kRows - 1) {
		cur_row_++;
	}
	else {
		// スクロールのために一度背景を塗りつぶす
		for (int y = 0; y < kRows * FONT_HEIGHT; y++) {
			for (int x = 0; x < kCols * FONT_WIDTH; x++) {
				pxwriter_.Write(x, y, c_bg_);
			}
		}
		for (int row = 0; row < kRows - 1; row++) {
			memcpy(buf_[row], buf_[row + 1], kCols + 1);
			WriteString(pxwriter_, 0, row * FONT_HEIGHT, buf_[row], c_fg_);
		}
		memset(buf_[kRows - 1], 0, kCols + 1);
	}
	return;
}