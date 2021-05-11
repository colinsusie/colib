/**
 * utf8的解码支持，用于遍历码点和取长度
 * 主体代码取自Lua的实现
 */
#ifndef _COUTF8_H
#define _COUTF8_H

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
int coutf8_len(const char *s);

#endif // _COUTF8_H

