#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/BlockIo.h>
#include <Guid/FileInfo.h>

#define	KERNEL_BASE_ADDRESS		0x110000

typedef struct _MemoryMap {
	UINTN	szbuf;
	VOID*	buf;
	UINTN	szmap;
	UINTN	mapkey;
	UINTN	szdesc;
	UINT32	verdesc;
} MEMMAP;

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
	CHAR8	buf[256];
	UINTN	len;
	int		i;

	CHAR8	*header = 
				"Index, Type, Type(name), PhysStart, NumOfPages, Attr\n";

	len = AsciiStrLen(header);
	file->Write(file, &len, header);

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
		file->Write(file, &len, buf);
	}
	return EFI_SUCCESS;
}

EFI_STATUS OpenRootDir(EFI_HANDLE himg, EFI_FILE_PROTOCOL **root) {
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL	*fs;
	EFI_LOADED_IMAGE_PROTOCOL		*ldimg;

	gBS->OpenProtocol(
		himg,
		&gEfiLoadedImageProtocolGuid,
		(VOID**)&ldimg,
		himg,
		NULL,
		EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
	);
	gBS->OpenProtocol(
		ldimg->DeviceHandle,
		&gEfiSimpleFileSystemProtocolGuid,
		(VOID**)&fs,
		himg,
		NULL,
		EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
	);
	fs->OpenVolume(fs, root);
	
	return EFI_SUCCESS;
}

EFI_STATUS UefiMain(EFI_HANDLE ImgHandle, EFI_SYSTEM_TABLE *SysTable) {
	EFI_FILE_PROTOCOL	*rootdir, *memmap_file;
	CHAR8	memmap_buf[4096 * 4];
	MEMMAP	memmap = { sizeof(memmap_buf), memmap_buf, 0, 0, 0, 0 };

	// kernel.elf 読み込み処理用
	EFI_FILE_PROTOCOL	*kernel_file;
	EFI_FILE_INFO		*finfo;
	UINTN	szfinfo = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 12;
	UINTN	szkernel_file;
	UINT8	finfo_buf[szfinfo];

	EFI_PHYSICAL_ADDRESS	kernel_baseaddr = KERNEL_BASE_ADDRESS;
	EFI_STATUS				status;
	UINT64					entry_addr;

	typedef void EntryPointType(void);

	Print(L"Welcome to Mysys\n");

	GetMemoryMap(&memmap);

	OpenRootDir(ImgHandle, &rootdir);
	rootdir->Open(
		rootdir, &memmap_file, L"\\memmap",
		EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0
	);
	SaveMemoryMap(&memmap, memmap_file);
	memmap_file->Close(memmap_file);

	Print(L"Saved Memory Map\n");

	rootdir->Open(rootdir, &kernel_file, L"\\kernel.elf", EFI_FILE_MODE_READ, 0);
	kernel_file->GetInfo(kernel_file, &gEfiFileInfoGuid, &szfinfo, finfo_buf);
	finfo = (EFI_FILE_INFO *)finfo_buf;
	szkernel_file = finfo->FileSize;

	gBS->AllocatePages(
		AllocateAddress, EfiLoaderData,
		(szkernel_file + 0xfff) / 0x1000, &kernel_baseaddr
	);
	kernel_file->Read(kernel_file, &szkernel_file, (VOID*)kernel_baseaddr);

	Print(L"Kernel is read to 0x%0lx (%lu bytes)\n", kernel_baseaddr, szkernel_file);

	status = gBS->ExitBootServices(ImgHandle, memmap.mapkey);

	if (EFI_ERROR(status)) {
		status = GetMemoryMap(&memmap);

		if (EFI_ERROR(status)) {
			Print(
				L"FATAL: Failed to GetMemoryMap at l%d in %s (status: %r)\n",
				__LINE__, __FILE__, status
			);
			while (1);
		}
		status = gBS->ExitBootServices(ImgHandle, memmap.mapkey);

		if (EFI_ERROR(status)) {
			Print(
				L"FATAL: Failed to ExitBootServices at l%d in %s (status: %r)\n",
				__LINE__, __FILE__, status
			);
			while (1);
		}
	}
	// ELFヘッダのオフセット24にエントリポイントのアドレスがある
	entry_addr = *(UINT64 *)(kernel_baseaddr + 24);
	((EntryPointType *)entry_addr)();

	Print(L"Why can you read this message...?\n");

	while (1);
	return EFI_SUCCESS;
}
