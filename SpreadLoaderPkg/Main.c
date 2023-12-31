#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/BlockIo.h>
#include <Guid/FileInfo.h>

#include "elf.hpp"
#include "framebuffer_config.hpp"

typedef struct _MemoryMap {
	UINTN	szbuf;
	VOID*	buf;
	UINTN	szmap;
	UINTN	mapkey;
	UINTN	szdesc;
	UINT32	verdesc;
} MEMMAP;

typedef void EntryPointType(const FrameBufferConfig *);

EFI_STATUS GetMemoryMap(MEMMAP *map) {
	if (map->buf == NULL) {
		return EFI_BUFFER_TOO_SMALL;
	}
	map->szmap = map->szbuf;

	return gBS->GetMemoryMap(
		&map->szmap,
		(EFI_MEMORY_DESCRIPTOR*)map->buf,
		&map->mapkey, &map->szdesc, &map->verdesc
	);
}

const CHAR16* GetMemoryTypeUnicode(EFI_MEMORY_TYPE type) {
	switch (type) {
	case EfiReservedMemoryType:		return L"EfiReservedMemoryType";
	case EfiLoaderCode:				return L"EfiLoaderCode";
	case EfiLoaderData:				return L"EfiLoaderData";
	case EfiBootServicesCode:		return L"EfiBootServicesCode";
	case EfiBootServicesData:		return L"EfiBootServicesData";
	case EfiRuntimeServicesCode:	return L"EfiRuntimeServicesCode";
	case EfiRuntimeServicesData:	return L"EfiRuntimeServicesData";
	case EfiConventionalMemory:		return L"EfiConventionalMemory";
	case EfiUnusableMemory:			return L"EfiUnusableMemory";
	case EfiACPIReclaimMemory:		return L"EfiACPIReclaimMemory";
	case EfiACPIMemoryNVS:			return L"EfiACPIMemoryNVS";
	case EfiMemoryMappedIO:			return L"EfiMemoryMappedIO";
	case EfiMemoryMappedIOPortSpace:return L"EfiMemoryMappedIOPortSpace";
	case EfiPalCode:				return L"EfiPalCode";
	case EfiPersistentMemory:		return L"EfiPersistentMemory";
	case EfiMaxMemoryType:			return L"EfiMaxMemoryType";
	default:						return L"InvalidMemoryType";
	}
}

EFI_STATUS SaveMemoryMap(MEMMAP *map, EFI_FILE_PROTOCOL *file) {
	EFI_PHYSICAL_ADDRESS	iter;
	EFI_STATUS	status;
	CHAR8		buf[256];
	UINTN		len;
	int			i;

	CHAR8	*header = 
				"Index, Type, Type(name), PhysStart, NumOfPages, Attr\n";

	len = AsciiStrLen(header);
	status = file->Write(file, &len, header);

	if (EFI_ERROR(status)) { return status; }

	Print(L"Memmap Buffer Address = %08x, Size = %08x\n", map->buf, map->szmap);

	for(iter = (EFI_PHYSICAL_ADDRESS)map->buf, i = 0;
		iter < (EFI_PHYSICAL_ADDRESS)map->buf + map->szmap;
		iter += map->szdesc, i++) {
		EFI_MEMORY_DESCRIPTOR	*desc = (EFI_MEMORY_DESCRIPTOR *)iter;

		len = AsciiSPrint(
			buf, sizeof(buf),
			"%u, %x, %-ls, %08lx, %lx, %lx\n",
			i, desc->Type, GetMemoryTypeUnicode(desc->Type),
			desc->PhysicalStart, desc->NumberOfPages, desc->Attribute & 0xffffflu
		);
		status = file->Write(file, &len, buf);

		if (EFI_ERROR(status)) { return status; }
	}
	return EFI_SUCCESS;
}

EFI_STATUS OpenRootDir(EFI_HANDLE himg, EFI_FILE_PROTOCOL **root) {
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL	*fs;
	EFI_LOADED_IMAGE_PROTOCOL		*ldimg;
	EFI_STATUS	status;

	status = gBS->OpenProtocol(
		himg,
		&gEfiLoadedImageProtocolGuid,
		(VOID**)&ldimg,
		himg,
		NULL,
		EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
	);
	if (EFI_ERROR(status)) { return status; }

	status = gBS->OpenProtocol(
		ldimg->DeviceHandle,
		&gEfiSimpleFileSystemProtocolGuid,
		(VOID**)&fs,
		himg,
		NULL,
		EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
	);
	if (EFI_ERROR(status)) { return status; }

	return fs->OpenVolume(fs, root);
}

EFI_STATUS OpenGOP(EFI_HANDLE himg, EFI_GRAPHICS_OUTPUT_PROTOCOL **gop) {
	EFI_STATUS	status;
	EFI_HANDLE	*hgop = NULL; 
	UINTN		nhgop = 0;

	status = gBS->LocateHandleBuffer(
		ByProtocol, &gEfiGraphicsOutputProtocolGuid,
		NULL, &nhgop, &hgop
	);
	if (EFI_ERROR(status)) { return status; }

	status = gBS->OpenProtocol(
		hgop[0], &gEfiGraphicsOutputProtocolGuid,
		(VOID **)gop, himg, NULL,
		EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL 
	);
	if (EFI_ERROR(status)) { return status; }

	FreePool(hgop);

	return EFI_SUCCESS;
}

const CHAR16* GetPixelFormatUnicode(EFI_GRAPHICS_PIXEL_FORMAT fmt) {
	switch (fmt) {
	case PixelRedGreenBlueReserved8BitPerColor:
		return L"PixelRedGreenBlueReserved8BitPerColor";
	case PixelBlueGreenRedReserved8BitPerColor:
		return L"PixelBlueGreenRedReserved8BitPerColor";
	case PixelBitMask:
		return L"PixelBitMask";
	case PixelBltOnly:
		return L"PixelBltOnly";
	case PixelFormatMax:
		return L"PixelFormatMax";
	default:
		return L"InvalidPixelFormat";
	}
}

// 全LOADセグメントのうち、最小開始アドレスと最大終端アドレスを求める
void CalcLoadAddressRange(ELF64_EHDR *ehdr, UINT64 *first, UINT64 *last) {
	ELF64_PHDR	*phdr = (ELF64_PHDR *)((UINT64)ehdr + ehdr->phoff);

	*first = MAX_UINT64;
	*last  = 0;

	for (ELF64_HALF i = 0; i < ehdr->phnum; i++) {
		if (phdr[i].type != PT_LOAD) continue;

		*first = MIN(*first, phdr[i].vaddr);
		*last  = MAX( *last, phdr[i].vaddr + phdr[i].szmem);
	}
	return;
}

void CopyLoadSegments(ELF64_EHDR *ehdr) {
	ELF64_PHDR	*phdr = (ELF64_PHDR *)((UINT64)ehdr + ehdr->phoff);
	UINT64		segm_in_file;
	UINTN		remain_bytes;

	for (ELF64_HALF i = 0; i < ehdr->phnum; i++) {
		if (phdr[i].type != PT_LOAD) continue;

		segm_in_file = (UINT64)ehdr + phdr[i].offset;
		CopyMem((VOID *)phdr[i].vaddr, (VOID *)segm_in_file, phdr[i].szfile);

		remain_bytes = phdr[i].szmem - phdr[i].szfile;
		SetMem((VOID *)(phdr[i].vaddr + phdr[i].szfile), remain_bytes, 0);
	}
}

void Halt(void) {
	while(1) { __asm__("hlt"); }
}

void CheckError(const CHAR16 *msg, EFI_STATUS status, int line) {
	if (EFI_ERROR(status)) {
		Print(L"FATAL: ");
		Print(msg);
		Print(L"\n at line %d, file %s (status: %r)\n", line, L""__FILE__, status);
		Halt();
	}
	return;
}

EFI_STATUS EFIAPI UefiMain(EFI_HANDLE ImgHandle, EFI_SYSTEM_TABLE *SysTable) {
	EFI_FILE_PROTOCOL	*rootdir, *memmap_file;
	CHAR8	memmap_buf[4096 * 4];
	MEMMAP	memmap = { sizeof(memmap_buf), memmap_buf, 0, 0, 0, 0 };

	EFI_GRAPHICS_OUTPUT_PROTOCOL	*gop;

	// kernel.elf 読み込み処理用
	EFI_FILE_PROTOCOL	*kernel_file;
	EFI_FILE_INFO		*finfo;
	UINTN	szfinfo = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 12;
	UINTN	szkernel_file;
	UINT8	finfo_buf[szfinfo];

	EFI_STATUS		status;
	UINT64			entry_addr;
	VOID			*kernel_buf;

	ELF64_EHDR	*kernel_ehdr;
	UINT64		kfirst_addr, klast_addr;
	UINTN		npages;

	Print(L"Welcome to Spread\n");

	// メモリマップの取得
	status = GetMemoryMap(&memmap);
	CheckError(L"Failed to GetMemoryMap", status, __LINE__);

	// memmap, kernel.elf 読み込みのためのデバイスオープン
	status = OpenRootDir(ImgHandle, &rootdir);
	CheckError(L"Failed to OpenRootDir", status, __LINE__);

	// memmap オープンとメモリマップ書き込み
	status = rootdir->Open(
		rootdir, &memmap_file, L"\\memmap",
		EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0
	);
	if (EFI_ERROR(status)) {
		Print(
			L"ERROR: Failed to open file \"\\memmap\" at line %d, file %s (status: %r)\n",
			__LINE__, __FILE__, status
		);
		Print(L"Booting can be still continued.\n");
	}
	else {
		status = SaveMemoryMap(&memmap, memmap_file);
		CheckError(L"Failed to SaveMemoryMap", status, __LINE__);
		status = memmap_file->Close(memmap_file);
		CheckError(L"Failed to close \"\\memmap\"", status, __LINE__);
	}
	// 画面出力（GOP）を扱えるようにする
	status = OpenGOP(ImgHandle, &gop);
	Print(
		L"Resolution: %ux%u, Pixel Format: %s, %u px / line\n",
		gop->Mode->Info->HorizontalResolution,
		gop->Mode->Info->VerticalResolution,
		GetPixelFormatUnicode(gop->Mode->Info->PixelFormat),
		gop->Mode->Info->PixelsPerScanLine
	);
	Print(
		L"Frame Buffer: 0x%0lx - 0x%0lx, (%lu bytes)",
		gop->Mode->FrameBufferBase,
		gop->Mode->FrameBufferBase + gop->Mode->FrameBufferSize,
		gop->Mode->FrameBufferSize
	);

	// kernel.elf 読み込み
	status = rootdir->Open(
		rootdir, &kernel_file, L"\\kernel.elf", EFI_FILE_MODE_READ, 0
	);
	CheckError(L"Failed to open \"\\kernel.elf\"", status, __LINE__);
	status = kernel_file->GetInfo(
		kernel_file, &gEfiFileInfoGuid, &szfinfo, finfo_buf
	);
	CheckError(L"Failed to get info of \"\\kernel.elf\"", status, __LINE__);

	finfo = (EFI_FILE_INFO *)finfo_buf;
	szkernel_file = finfo->FileSize;
	
	status = gBS->AllocatePool(EfiLoaderData, szkernel_file, &kernel_buf);
	CheckError(L"Failed to allocate pool", status, __LINE__);
	status = kernel_file->Read(kernel_file, &szkernel_file, kernel_buf);
	CheckError(L"Failed to read file \"\\kernel.elf\"", status, __LINE__);

	// kernel.elf の ELF ヘッダ解析と再配置メモリ確保
	kernel_ehdr = (ELF64_EHDR *)kernel_buf;
	CalcLoadAddressRange(kernel_ehdr, &kfirst_addr, &klast_addr);
	npages = (klast_addr - kfirst_addr + 0xfff) / 0x1000;

	status = gBS->AllocatePages(
		AllocateAddress, EfiLoaderData, npages, &kfirst_addr
	);
	CheckError(L"Failed to allocate pages", status, __LINE__);

	// カーネルコードを再配置場所にコピー
	CopyLoadSegments(kernel_ehdr);
	Print(
		L"Kernel is loaded to 0x%0lx - 0x%0lx (%lu bytes)\n",
		kfirst_addr, klast_addr, klast_addr - kfirst_addr
	);
	status = gBS->FreePool(kernel_buf);
	CheckError(L"Failed to free pool", status, __LINE__);

	// UEFI を出る
	status = gBS->ExitBootServices(ImgHandle, memmap.mapkey);

	if (EFI_ERROR(status)) {
		status = GetMemoryMap(&memmap);
		CheckError(L"Failed to GetMemoryMap", status, __LINE__);

		status = gBS->ExitBootServices(ImgHandle, memmap.mapkey);
		CheckError(L"Failed to exit BootServices", status, __LINE__);
	}

	// カーネル側にグラフィックス情報を渡すための準備
	FrameBufferConfig conf = {
		(UINT8 *)gop->Mode->FrameBufferBase,
		gop->Mode->Info->PixelsPerScanLine,
		gop->Mode->Info->HorizontalResolution,
		gop->Mode->Info->VerticalResolution,
		0
	};
	switch (gop->Mode->Info->PixelFormat)
	{
	case PixelRedGreenBlueReserved8BitPerColor:
		conf.pixelformat = kPixelRGBResv8BitPerColor; break;
	case PixelBlueGreenRedReserved8BitPerColor:
		conf.pixelformat = kPixelBGRResv8BitPerColor; break;
	default:
		Print(L"FATAL: Unimplemented pixel format: %d\n", gop->Mode->Info->PixelFormat);
		Halt();
	}
	// ELFヘッダのオフセット24にエントリポイントのアドレスがある
	entry_addr = *(UINT64 *)(kfirst_addr + 24);
	((EntryPointType *)entry_addr)(&conf);
	Print(L"Why can you read this message...?\n");

	while (1);
	return EFI_SUCCESS;
}
