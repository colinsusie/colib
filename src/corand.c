#include "corand.h"

/* rotate left 'x' by 'n' bits */
static inline uint64_t _rotleft(uint64_t x, int n) {
	return (x << n) | (x >> (64 - n));
}

void randinit(randstate_t *state, uint64_t seed1, uint64_t seed2) {
	uint64_t *s = state->s;
	s[0] = seed1;
	s[1] = 0xff;
	s[2] = seed2;
	s[3] = 0;
}

// 生成下一个随机数，范围是：[0, 2^64)
uint64_t randnext(randstate_t *state) {
	uint64_t *s = state->s;
	const uint64_t result = _rotleft(s[1] * 5, 7) * 9;
	const uint64_t t = s[1] << 17;
	s[2] ^= s[0];
	s[3] ^= s[1];
	s[1] ^= s[2];
	s[0] ^= s[3];
	s[2] ^= t;
	s[3] = _rotleft(s[3], 45);
	return result;
}