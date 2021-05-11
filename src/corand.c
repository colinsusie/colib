#include "corand.h"

/* rotate left 'x' by 'n' bits */
static inline uint64_t _rotleft(uint64_t x, int n) {
	return (x << n) | (x >> (64 - n));
}

void randseed(randstate_t *state, uint64_t seed1, uint64_t seed2) {
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

static uint64_t project(uint64_t ran, uint64_t n, randstate_t *state) {
	if ((n & (n + 1)) == 0)  /* is 'n + 1' a power of 2? */
		return ran & n;  /* no bias */
	else {
		uint64_t lim = n;
		/* compute the smallest (2^b - 1) not smaller than 'n' */
		lim |= (lim >> 1);
		lim |= (lim >> 2);
		lim |= (lim >> 4);
		lim |= (lim >> 8);
		lim |= (lim >> 16);
		lim |= (lim >> 32);
		while ((ran &= lim) > n)  /* project 'ran' into [0..lim] */
			ran = randnext(state);  /* not inside [0..n]? try again */
		return ran;
	}
}

// 生成整型随机数，范围是：[a, b]
int64_t randintrange(randstate_t *state, int64_t a, int64_t b) {
	if (a == b) return a;
	uint64_t low = a < b ? a : b;
	uint64_t up = a < b ? b : a;
	uint64_t rv = randnext(state);
	uint64_t p = project(rv, up - low, state);
	return (int64_t)(p + low);
}