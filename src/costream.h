/**
 * 提供统一的流接口
 */
#ifndef _COSTREAM_H
#define _COSTREAM_H
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "coconf.h"

//-----------------------------------------------------------------------------
// istream

// 流读取函数
typedef const char* (*stream_reader_f)(void *ud, size_t *sz);

// 输入流
typedef struct istream {
	const char *p;		// 当前buff块的读取位置
	const char *b;		// 当前buff块
	size_t n;			// 当前buff块还剩多少
	size_t bn;			// 已获得的buff块大小
	stream_reader_f reader;
	void *ud;
} istream_t;

// 初始化输入流
static inline void istream_init(istream_t *stm, stream_reader_f reader,  void *ud) {
	assert(reader != NULL);
	stm->reader = reader;
	stm->ud = ud;
	stm->p = NULL;
	stm->b = NULL;
	stm->n = 0;
	stm->bn = 0;
}

int _istream_fill(istream_t *stm);

// 取一个字符
static inline int istream_getc(istream_t *stm) {
	if (likely(stm->n-- > 0)) {
		return (unsigned char)*stm->p++;
	} else if (_istream_fill(stm)) {
		stm->n--;
		return *stm->p++;
	} else
		return EOF;
}

// 取流的当前的读位置
static inline size_t istream_pos(istream_t *stm) {
	return stm->bn - stm->n;
}

// 取n个字节
size_t istream_getb(istream_t *stm, void *b, size_t n);

//---------------------------------
// istrstream

// 字符串输入流
typedef struct strreader {
	const char *str;
	size_t sz;
} strreader_t;

// 从字符串输入流读内容
const char* strreader_read(void *ud, size_t *sz);

// 初始化字符串输入流
static inline void istrstream_init(istream_t *stm, strreader_t *sreader, const char *str, size_t sz) {
	sreader->str = str;
	sreader->sz = sz;
	istream_init(stm, strreader_read, sreader);
}

//---------------------------------
// ifilestream

#define FR_MIN_BUFFSZ 64

//  文件输入流
typedef struct filereader {
	FILE *f;		// 文件句柄
	char *b;		// 缓存
	size_t bsz;		// 缓存的大小
	size_t n;		// 缓存的可用大小
} filereader_t;

// 从文件输入流读内容
const char* filereader_read(void *ud, size_t *sz);

// 初始化文件输入流
static inline void ifilestream_init(istream_t *stm, filereader_t *freader, FILE* f, size_t bsz) {
	freader->f = f;
	freader->n = 0;
	freader->bsz = bsz < FR_MIN_BUFFSZ ? FR_MIN_BUFFSZ : bsz;
	freader->b = (char*)co_malloc(bsz);
	istream_init(stm, filereader_read, freader);
}

// 初始化文件输入流, bsz指定缓冲的大小，如果为0表示预加载整个文件内容(最多2G)
static inline int ifilestream_initf(istream_t *stm, filereader_t *freader, const char *fname, int bsz) {
    freader->f = NULL;
	freader->b = NULL;
	FILE *f = fopen(fname, "rb");
	if (f != NULL) {
		ifilestream_init(stm, freader, f, bsz);
		return 1;
	}
	return 0;
}

// 关闭文件输入流
static inline int ifilestream_close(filereader_t *freader) {
	// printf("ifilestream_close, %p, %p\n", freader->b, freader->f);
	if (freader->b) {
		co_free(freader->b);
		freader->b = NULL;
	}
	if (freader->f) {
		int ret = fclose(freader->f);
		freader->f = NULL;
		return ret;
	}
	return 1;
}

// 略过UTF8的BOM头
void ifilestream_skipbom(filereader_t *freader);

//-----------------------------------------------------------------------------
// membuffer

#define STACK_BUFF_SIZE 512

typedef struct membuffer {
	char *b;		// 内存buffer
	size_t sz;		// buffer已用长度
	size_t cap;		// buffer实际大小
	char s[STACK_BUFF_SIZE];
} membuffer_t;

// 初始化buffer
static inline void membuffer_init(membuffer_t *buff) {
	buff->b = buff->s;
	buff->cap = STACK_BUFF_SIZE;
	buff->sz = 0;
}

static inline void membuffer_remove_size(membuffer_t *buff, size_t sz) {
	buff->sz -= sz;
}

static inline void membuffer_add_size(membuffer_t *buff, size_t sz) {
	buff->sz += sz;
}

static inline void membuffer_reset(membuffer_t *buff) {
	buff->sz = 0;
}

static inline void membuffer_free(membuffer_t *buff) {
	if (buff->b && buff->b != buff->s) {
		co_free(buff->b);
		buff->b = NULL;
	}
}

static inline void _membuffer_grow(membuffer_t *buff, size_t needsz) {
	if (buff->cap < needsz) {
		size_t newcap = buff->cap * 2;
		if (newcap < needsz)
			newcap = needsz;
		if (buff->b == buff->s) {
			buff->b = (char*)co_malloc(newcap);
			memcpy(buff->b, buff->s, buff->sz);
		} else {
			buff->b = (char*)co_realloc(buff->b, newcap);
		}
		buff->cap = newcap;
	}
}

// 确保缓存中还有sz的可用空间
static inline void membuffer_ensure_space(membuffer_t *buff, size_t sz) {
	if (buff->sz + sz > buff->cap) {
		_membuffer_grow(buff, buff->sz+sz);
	}
}

// 压入一个字符
static inline void membuffer_putc(membuffer_t *buff, char c) {
	membuffer_ensure_space(buff, 1);
	buff->b[buff->sz++] = c;
}

// 写入一段内存
static inline void membuffer_putb(membuffer_t *buff, const void *b, size_t sz) {
	membuffer_ensure_space(buff, sz);
	memcpy(buff->b + buff->sz, b, sz);
	buff->sz += sz;
}

// 压入一个字符：不检查空间(不安全版本)
static inline void membuffer_putc_unsafe(membuffer_t *buff, char c) {
	buff->b[buff->sz++] = c;
}

// 写入一段内存：不检查空间(不安全版本)
static inline void membuffer_putb_unsafe(membuffer_t *buff, const void *b, size_t sz) {
	memcpy(buff->b + buff->sz, b, sz);
	buff->sz += sz;
}

// 取当前的指针
static inline char* membuffer_getp(membuffer_t *buff) {
	return buff->b + buff->sz;
}


#endif