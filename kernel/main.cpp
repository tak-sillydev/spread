#include <cstdint>

extern "C" void KernelMain(uint64_t framebuf_base, uint64_t framebuf_size) {
	uint8_t *framebuf = reinterpret_cast<uint8_t *>(framebuf_base);

	for (uint64_t i = 0; i < framebuf_size; i++) {
		framebuf[i] = i % 256;
	}
	while (1) { __asm__("hlt"); }
}
