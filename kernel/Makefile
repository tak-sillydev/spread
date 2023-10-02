TARGET = kernel.elf
OBJS = main.o graphics.o font.o hankaku.o newlib_support.o console.o \
		pci.o asmfunc.o
DEPENDS = $(join $(dir $(OBJS)), $(addprefix ., $(notdir $(OBJS:.o=.d))))
# join   : 引数１と引数２の要素をインデックス順に結合し１つの文字列として配列化。
# dir    : 引数の要素内の文字列から「ディレクトリ名」を抽出。
# addprefix : 引数２の要素の文字列の先頭に引数１の文字列を結合する。
# notdir : 引数の要素内の文字列から「ファイル名」を抽出。
# >> *.o -> .*.d -> /path/.*.d を OBJS の各要素に対して実行する。

CFLAGS   += -O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone
CXXFLAGS += -O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone \
			-fno-exceptions -fno-rtti -std=c++17
LDFLAGS  += --entry KernelMain -z norelro --image-base 0x110000 --static


.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	rm -rf *.o

kernel.elf: $(OBJS) Makefile
	ld.lld $(LDFLAGS) -o kernel.elf $(OBJS) -lc

%.o: %.cpp Makefile
	clang++ $(CPPFLAGS) $(CXXFLAGS) -c $<

%.o: %.c Makefile
	clang $(CPPFLAGS) $(CFLAGS) -c $<

%.o: %.asm Makefile
	nasm -f elf64 -o $@ $<

.%.d: %.cpp
	clang++ $(CPPFLAGS) $(CXXFLAGS) -MM $< > $@
	$(eval OBJ = $(<:.cpp=.o))
	sed --in-place 's|$(notdir $(OBJ))|$(OBJ)|' $@
# -MM  : Makefile 使用時専用。Makefile の書式でヘッダファイルの依存関係を出力。
#        ただしコンパイルは行わない。結果を $@ (.%.d) へリダイレクト。
# eval : 動的に Makefile の命令を生成、実行できる。
#        >> OBJ = ($< の .cpp を .o に変換したもの)
# --in-place : 標準出力には出力せず、ファイルの上書きのみ行う。
# OBJ のファイル名を見つけたらパス付きのものに置き換える（はじめの１行のみ）。

.%.d: %.c
	clang $(CPPFLAGS) $(CFLAGS) -MM $< > $@
	$(eval OBJ = $(<:.c=.o))
	sed --in-place 's|$(notdir $(OBJ))|$(OBJ)|' $@

.%.d: %.bin
	touch $@

hankaku.bin: hankaku.txt Makefile
	../tools/makefont.py -o $@ $<

hankaku.o: hankaku.bin Makefile
	objcopy -I binary -O elf64-x86-64 -B i386:x86-64 $< $@

.PHONY: depends
depends:
	$(MAKE) $(DEPENDS)

-include $(DEPENDS)
