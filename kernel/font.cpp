#include "font.hpp"

const uint8_t kFontA[16] = {
	0b00000000,	//
	0b00011000,	//    **   
	0b00011000,	//    **   
	0b00011000,	//    **   
	0b00011000,	//    **   
	0b00100100,	//   *  *  
	0b00100100,	//   *  *  
	0b00100100,	//   *  *  
	0b00100100,	//   *  *  
	0b01111110,	//  ****** 
	0b01000010,	//  *    * 
	0b01000010,	//  *    * 
	0b01000010,	//  *    * 
	0b11100111,	// ***  *** 
	0b00000000,	//
	0b00000000,	//
};

void WriteAscii(PixelWriter& pxwriter, int x, int y, char ch, const PixelColor& c) {
	if (ch != 'A') { return; }

	for (int dy = 0; dy < 16; dy++) {
		for (int dx = 0; dx < 8; dx++) {
			if ((kFontA[dy] << dx) & 0x80u) {
				pxwriter.Write(x + dx, y + dy, c);
			}
		}
	}
}

