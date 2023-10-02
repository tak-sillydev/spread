// pci.cpp
// PCI バス制御関連

#include "pci.hpp"
#include "asmfunc.h"

// 名前無し名前空間（？）は別のファイルのコードからは参照不可
namespace {
	using namespace PCI;

	Error ScanBus(uint8_t bus);

	uint32_t MakeAddress(uint8_t bus, uint8_t dev, uint8_t func, uint8_t regaddr) {
		auto shl = [](uint32_t x, unsigned int bits) { return x << bits; };

		return shl(1, 31) | shl(bus, 16) | shl(dev, 11) | shl(func, 8) | (regaddr & 0xfcu);
	}

	Error AddDevice(uint8_t bus, uint8_t dev, uint8_t func, uint8_t hdrtype) {
		if (ndevs == devices.size()) {
			return Error::kFull;
		}
		devices[ndevs++] = Device{ bus, dev, func, hdrtype };

		return Error::kSuccess;
	}

	Error ScanFunction(uint8_t bus, uint8_t dev, uint8_t func) {
		uint8_t	base, sub;
		auto	hdrtype = ReadHeaderType(bus, dev, func);

		if (auto err = AddDevice(bus, dev, func, hdrtype)) {
			return err;
		}
		auto clscode = ReadClassCode(bus, dev, func);

		base = (clscode >> 24) & 0xffu;
		sub  = (clscode >> 16) & 0xffu;

		if (base == 0x06u && sub == 0x04u) {
			// standard PCI-PCI bridge
			auto	busnums = ReadBusNumbers(bus, dev, func);
			uint8_t	secondary_bus = (busnums >> 8) & 0xffu;

			return ScanBus(secondary_bus);
		}
		return Error::kSuccess;
	}

	Error ScanDevice(uint8_t bus, uint8_t dev) {
		if (auto err = ScanFunction(bus, dev, 0)) { return err; }
		if (IsSingleFunctionDevice(ReadHeaderType(bus, dev, 0))) {
			return Error::kSuccess;
		}
		for (uint8_t func = 1; func < kMaxFunction; func++) {
			if (ReadVendorId(bus, dev, func) == kInvalidVendorId) { continue; }
			if (auto err = ScanFunction(bus, dev, func)) { return err; }
		}
		return Error::kSuccess;
	}

	Error ScanBus(uint8_t bus) {
		for (uint8_t dev = 0; dev < kMaxDevices; dev++) {
			if (ReadVendorId(bus, dev, 0) == kInvalidVendorId) { continue; }
			if (auto err = ScanDevice(bus, dev)) { return err; }
		}
		return Error::kSuccess;
	}
}

namespace PCI {
	void WriteAddress(uint32_t addr) { IoOut32(kConfigAddress, addr); return; }
	void WriteData(uint32_t val) { IoOut32(kConfigData, val); return; }
	uint32_t ReadData() { return IoIn32(kConfigData); }

	uint16_t ReadVendorId(uint8_t bus, uint8_t dev, uint8_t func) {
		WriteAddress(MakeAddress(bus, dev, func, 0x00));
		return ReadData() & 0xffffu;
	}

	uint16_t ReadDeviceId(uint8_t bus, uint8_t dev, uint8_t func) {
		WriteAddress(MakeAddress(bus, dev, func, 0x00));
		return (ReadData() >> 16) & 0xffffu;
	}

	uint8_t ReadHeaderType(uint8_t bus, uint8_t dev, uint8_t func) {
		WriteAddress(MakeAddress(bus, dev, func, 0x0c));
		return (ReadData() >> 16) & 0xffu;
	}

	// クラスコードの読み取り。
	// 31-24: ベースクラス、23-16: サブクラス、15-8: インターフェース、7-0: リビジョンID
	uint32_t ReadClassCode(uint8_t bus, uint8_t dev, uint8_t func) {
		WriteAddress(MakeAddress(bus, dev, func, 0x08));
		return ReadData();
	}

	// PCI デバイスが PCI-PCI ブリッジの場合に用いる（ヘッダタイプ 1 用）
	// 23-16: サブオーディネイトバス番号、15-8: セカンダリバス番号、7-0: リビジョン番号
	uint32_t ReadBusNumbers(uint8_t bus, uint8_t dev, uint8_t func) {
		WriteAddress(MakeAddress(bus, dev, func, 0x18));	// BAR2
		return ReadData();
	}

	// デバイスから取得したヘッダタイプを指定する。
	bool IsSingleFunctionDevice(uint8_t hdrtype) { return !(hdrtype & 0x80u); }

	Error ScanAllBus() {
		ndevs = 0;

		auto hdrtype = ReadHeaderType(0, 0, 0);

		// 00:00:0 はホストブリッジであり、ファンクション番号が実装バス番号に対応
		if (IsSingleFunctionDevice(hdrtype)) {
			return ScanBus(0);
		}
		for (uint8_t func = 1; func < kMaxFunction; func++) {
			if (ReadVendorId(0, 0, func) == kInvalidVendorId) { continue; }
			if (auto err = ScanBus(func)) { return err; }
		}
		return Error::kSuccess;
	}
}