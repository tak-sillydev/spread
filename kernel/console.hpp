#pragma once

#include "graphics.hpp"

class Console {
public:
	static const int kRows = 25, kCols = 80;

	Console(PixelWriter& pxwriter, const PixelColor& c_fg, const PixelColor& c_bg);
	void PutString(const char *s);

private:
	void NewLine();

	PixelWriter& 		pxwriter_;
	const PixelColor	c_fg_, c_bg_;	// 前景色、背景色
	char	buf_[kRows][kCols + 1];		// 出力記憶用バッファ
	int		cur_row_, cur_col_;			// 行と列で表されるカーソル位置
};
