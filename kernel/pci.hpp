#pragma once

#include <cstdint>
#include <array>

#include "error.hpp"

namespace PCI {
	// PCI アクセス用の I/O アドレス
	constexpr uint16_t	kConfigAddress	= 0x0cf8;
	constexpr uint16_t	kConfigData		= 0x0cfc;

	constexpr int		kMaxDevices			= 32;
	constexpr int		kMaxFunction		= 8;
	constexpr uint16_t	kInvalidVendorId	= 0xffffu;

	struct Device {
		uint8_t	bus, dev, func, hdrtype;
	};
	inline std::array<Device, 32>	devices;
	inline int	ndevs;

	void WriteAddress(uint32_t addr);
	void WriteData(uint32_t val);
	uint32_t ReadData();

	uint16_t ReadVendorId(uint8_t bus, uint8_t dev, uint8_t func);
	uint16_t ReadDeviceId(uint8_t bus, uint8_t dev, uint8_t func);
	uint8_t  ReadHeaderType(uint8_t bus, uint8_t dev, uint8_t func);
	uint32_t ReadClassCode(uint8_t bus, uint8_t dev, uint8_t func);
	uint32_t ReadBusNumbers(uint8_t bus, uint8_t dev, uint8_t func);
	bool IsSingleFunctionDevice(uint8_t hdrtype);

	Error ScanAllBus();
} // namespace PCI
