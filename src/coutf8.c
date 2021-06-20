#include "coutf8.h"
#include <stdlib.h>
#include <assert.h>

#define MAXUNICODE 0x10FFFFu
#define MAXUTF 0x7FFFFFFFu

const char* coutf8_decode(const char* s, utfint* val, int strict) {
	static const utfint limits[] = { ~(utfint)0, 0x80, 0x800, 0x10000u, 0x200000u, 0x4000000u };
	unsigned int c = (unsigned char)s[0];
	utfint res = 0; /* final result */
	if (c < 0x80) /* ascii? */
		res = c;
	else {
		int count = 0; /* to count number of continuation bytes */
		for (; c & 0x40; c <<= 1) { /* while it needs continuation bytes... */
			unsigned int cc = (unsigned char)s[++count]; /* read next byte */
			if ((cc & 0xC0) != 0x80) /* not a continuation byte? */
				return NULL; /* invalid byte sequence */
			res = (res << 6) | (cc & 0x3F); /* add lower 6 bits from cont. byte */
		}
		res |= ((utfint)(c & 0x7F) << (count * 5)); /* add first byte */
		if (count > 5 || res > MAXUTF || res < limits[count])
			return NULL; /* invalid byte sequence */
		s += count; /* skip continuation bytes read */
	}
	if (strict) {
		/* check for invalid code points; too large or surrogates */
		if (res > MAXUNICODE || (0xD800u <= res && res <= 0xDFFFu))
			return NULL;
	}
	if (val)
		*val = res;
	return s + 1; /* +1 to include first byte */
}

int coutf8_len(const char* s) {
	int count = 0;
	const char *p = s;
	while (*p) {
		p = coutf8_decode(p, NULL, 1);
		if (!p) break;
		++count;
	}
	return count;
}

int coutf8_encode(char *buff, unsigned long x) {
	int n = 1;  /* number of bytes put in buffer (backwards) */
	assert(x <= 0x7FFFFFFFu);
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