#include "costream.h"

#include "costream.h"

int _istream_fill(istream_t *stm) {
	size_t sz;
	const char *b = stm->reader(stm->ud, &sz);
	if (b && sz > 0) {
		stm->bn += sz;
		stm->n = sz;
		stm->b = b;
		stm->p = b;
		return 1;
	} else {
		stm->n = 0;
	}
	return 0;
}

size_t istream_getb(istream_t *stm, void *b, size_t n) {
	size_t m;
	size_t c = n;
	while (c) {
		if (stm->n == 0 && !_istream_fill(stm))
			return n - c;
		m = (c <= stm->n) ? c : stm->n;
		memcpy(b, stm->p, m);
		stm->n-= m;
		stm->p += m;
		b = (char*)b + m;
		c -=m;
	}
	return n;
}

const char* strreader_read(void *ud, size_t *sz) {
	strreader_t *sreader = (strreader_t*)ud;
	if (sreader->sz == 0) return NULL;
	*sz = sreader->sz;
	sreader->sz = 0;
	return sreader->str;
}

const char* filereader_read(void *ud, size_t *sz) {
	filereader_t *freader = (filereader_t*)ud;
	if (freader->n > 0) {
		*sz = freader->n;
		freader->n = 0;
	} else {
		if (feof(freader->f)) return NULL;
		*sz = fread(freader->b, 1, freader->bsz, freader->f);
	}
	return freader->b;
}

void ifilestream_skipbom(filereader_t *freader) {
	const char *p = "\xEF\xBB\xBF";  /* UTF-8 BOM mark */
	int c;
	do {
		c = getc(freader->f);
		if (c == EOF) 
			return;
		freader->b[freader->n++] = c;
		if (c != (unsigned char)*p) 
			return;
	} while (*(++p) != '\0');
	freader->n = 0;		// 匹配前缀，把前缀丢掉
}