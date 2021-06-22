/**
 * utf8的解码支持，用于遍历码点和取长度
 * 主体代码取自Lua的实现
 */
#ifndef _COUTF8_H
#define _COUTF8_H
#include <stdlib.h>

/*
** Integer type for decoded UTF-8 values; MAXUTF needs 31 bits.
*/
#if (UINT_MAX >> 30) >= 1
typedef unsigned int utfint;
#else
typedef unsigned long utfint;
#endif

// 解码一个utf8字节序列
// @param val utfint 解码出来的值，可以为NULL
// @param strict bool 是否为严格模式，为1时不接受大于10FFFF的值，一般传1即可
// @return 如果格式错误返回NULL；val为返回的码点，
const char *coutf8_decode(const char *s, utfint *val, int strict);

// 计算一个utf8字符串的长度
static inline int coutf8_len(const char* s) {
	int count = 0;
	const char *p = s;
	while (*p) {
		p = coutf8_decode(p, NULL, 1);
		if (!p) break;
		++count;
	}
	return count;
}

#define UTF8BUFFSZ	8

// 编码unicode codepoint为utf8序列
// int coutf8_encode(char *buff, unsigned long x);

static inline int coutf8_encode(char *buff, unsigned long x) {
	int n = 1;  /* number of bytes put in buffer (backwards) */
	if (x < 0x80)  /* ascii? */
		buff[UTF8BUFFSZ - 1] = (char)x;
	else {  /* need continuation bytes */
		unsigned int mfb = 0x3f;  /* maximum that fits in first byte */
		do {  /* add continuation bytes */
			buff[UTF8BUFFSZ - (n++)] = (char)(0x80 | (x & 0x3f));
			x >>= 6;  /* remove added bits */
			mfb >>= 1;  /* now there is one less bit available in first byte */
		} while (x > mfb);  /* still needs continuation byte? */
		buff[UTF8BUFFSZ - n] = (char)((~mfb << 1) | x);  /* add first byte */
	}
	return n;
}

#endif // _COUTF8_H

