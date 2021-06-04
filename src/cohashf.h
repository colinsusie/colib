/**
 * 各种非加密哈希函数
 */
#ifndef _COHASHF_H
#define _COHASHF_H
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>

static inline uint64_t hashf_fnv1a_64(const uint8_t *data, size_t size) {
	uint64_t hash = 14695981039346656037ULL;
	size_t i;
	for (i = 0; i < size; ++i) {
		hash ^= (uint64_t)*data++;
		hash *= 1099511628211ULL;
	}
	return hash;
}

static inline uint32_t hashf_fnv1a_32(const uint8_t *data, size_t size) {
	uint32_t hash = 2166136261UL;
	while (size--) {
		hash ^= (uint32_t)*data++;
		hash *= 16777619UL;
	}
	return hash;
}

static inline uint32_t hashf_djb2(const uint8_t *data, size_t size) {
	uint32_t hash = 5381;
	int c;
	while (size--) {
		c = *data++;
		hash = ((hash << 5) + hash) + c; 	/* hash * 33 + c */
	}
	return hash;
}

static inline uint32_t hashf_sdbm(const uint8_t *data, size_t size) {
	uint32_t hash = 0;
	int c;
	while (size--) {
		c = *data++;
		hash = c + (hash << 6) + (hash << 16) - hash;
	}
	return hash;
}

static inline uint32_t hashf_rs(const uint8_t *data, size_t size) {
	uint32_t b    = 378551;
	uint32_t a    = 63689;
	uint32_t hash = 0;
	int c;
	while (size--) {
		c = *data++;
		hash = hash * a + c;
		a  = a * b;
	}
	return hash;
}

static inline uint32_t hashf_js(const uint8_t *data, size_t size) {
   uint32_t hash = 1315423911;
   int c;
	while (size--) {
		c = *data++;
		hash ^= ((hash << 5) + c + (hash >> 2));
	}
	return hash;
}

static inline uint32_t hashf_bkdr(const uint8_t *data, size_t size) {
	uint32_t seed = 131; /* 31 131 1313 13131 131313 etc.. */
	uint32_t hash = 0;
	int c;
	while (size--) {
		c = *data++;
		hash = (hash * seed) + c;
	}
	return hash;
}

static inline uint32_t hashf_dek(const uint8_t *data, size_t size) {
	uint32_t hash = size;
	int c;
	while (size--) {
		c = *data++;
		hash = ((hash << 5) ^ (hash >> 27)) ^ c;
	}
	return hash;
}

static inline uint32_t hashf_ap(const uint8_t *data, size_t size) {
	uint32_t hash = 0xAAAAAAAA;
	size_t i = 0;
	for (i = 0; i < size; ++data, ++i) {
		hash ^= ((i & 1) == 0) ? (  (hash <<  7) ^ (*data) * (hash >> 3)) :
								(~((hash << 11) + ((*data) ^ (hash >> 5))));
	}
	return hash;
}

#endif