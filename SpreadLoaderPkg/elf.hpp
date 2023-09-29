#pragma once

#include <stdint.h>

typedef uintptr_t	ELF64_ADDR;
typedef uint64_t	ELF64_OFFSET;
typedef uint16_t	ELF64_HALF;
typedef	uint32_t	ELF64_WORD;
typedef int32_t		ELF64_SWORD;
typedef uint64_t	ELF64_XWORD;
typedef int64_t		ELF64_SXWORD;

#define	EI_NIDENT	16

typedef struct {
	unsigned char	ident[EI_NIDENT];
	ELF64_HALF		type;
	ELF64_HALF		machine;
	ELF64_WORD		version;
	ELF64_ADDR		entry;
	ELF64_OFFSET	phoff;		// プログラムヘッダ開始オフセット
	ELF64_OFFSET	shoff;		// セクションヘッダ開始オフセット
	ELF64_WORD		flags;
	ELF64_HALF		ehsize;
	ELF64_HALF		phentsize;
	ELF64_HALF		phnum;		// プログラムヘッダ配列要素数
	ELF64_HALF		shentsize;
	ELF64_HALF		shnum;		// セクションヘッダ配列要素数
	ELF64_HALF		shstrndx;
} ELF64_EHDR;

typedef struct {
	ELF64_WORD		type;		// セグメント種別（PHDR, LOAD 等）
	ELF64_WORD		flags;		// フラグ
	ELF64_OFFSET	offset;		// オフセット
	ELF64_ADDR		vaddr;		// 仮想アドレス
	ELF64_ADDR		paddr;		// 物理アドレス
	ELF64_XWORD		szfile;		// ファイルサイズ
	ELF64_XWORD		szmem;		// メモリサイズ
	ELF64_XWORD		align;
} ELF64_PHDR;

#define	PT_NULL		0
#define	PT_LOAD		1
#define	PT_DYNAMIC	2
#define	PT_INTERP	3
#define	PT_NOTE		4
#define	PT_SHLIB	5
#define	PT_PHDR		6
#define	PT_TLS		7

typedef struct {
	ELF64_SXWORD	tag;
	union {
		ELF64_XWORD	val;
		ELF64_ADDR	ptr;
	} un;
} ELF64_DYN;

#define	DT_NULL		0
#define	DT_RELA		7
#define	DT_RELASZ	8
#define	DT_RELAENT	9

typedef struct {
	ELF64_ADDR		offset;
	ELF64_XWORD		info;
	ELF64_SXWORD	addend;
} ELF64_RELA;

#define	ELF64_R_SYM(i)		((i) >> 32)
#define	ELF64_R_TYPE(i)		((i) & 0xffffffffL)
#define	ELF64_R_INFO(s, t)	(((s) << 32) + ((t) & 0xffffffffL))

#define	R_X86_64_RELATIVE	8
