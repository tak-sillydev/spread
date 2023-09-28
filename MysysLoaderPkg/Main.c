#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/BlockIo.h>

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

	while (1);
	return EFI_SUCCESS;
}
