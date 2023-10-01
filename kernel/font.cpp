// font.cpp
// hankaku.txt から生成されたシステム フォントを操作する。

#include "font.hpp"

// objcopy コマンドで生成されたバイナリデータを表す変数は「配列」として渡される。
// size だけはアドレスがそのまま値として使われるので注意。（オブジェクトの終端に変数を配置したイメージ？）
extern const uint8_t	_binary_hankaku_bin_start[];
extern const uint8_t	_binary_hankaku_bin_end[];
extern const uint8_t	_binary_hankaku_bin_size[];

const uint8_t* GetFont(char ch) {
	auto ndx = FONT_BINARY_SIZE * static_cast<uint64_t>(ch);

	return ndx > reinterpret_cast<uint64_t>(_binary_hankaku_bin_size) ?
			nullptr : _binary_hankaku_bin_start + ndx;
}

void WriteAscii(PixelWriter& pxwriter, int x, int y, char ch, const PixelColor& c) {
	const uint8_t	*font = GetFont(ch);

	if (font) {
		for (int dy = 0; dy < FONT_HEIGHT; dy++) {
			for (int dx = 0; dx < FONT_WIDTH; dx++) {
				if ((font[dy] << dx) & 0x80u) {
					pxwriter.Write(x + dx, y + dy, c);
				}
			}
		}
	}
}

void WriteString(PixelWriter& pxwriter, int x, int y, const char *s, const PixelColor& c) {
	for (int i = 0; s[i] != '\0'; i++) {
		WriteAscii(pxwriter, x + i * FONT_WIDTH, y, s[i], c);
	}
	return;
}
