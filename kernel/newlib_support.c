// newlib_support.c
// フリースタンディング環境向け標準関数ライブラリ "Newlib" が
// 各ホスト環境に実装を求める関数群。

#include <errno.h>
#include <sys/types.h>

caddr_t sbrk(int incr) {
	errno = ENOMEM;

	return (caddr_t)-1;
}
