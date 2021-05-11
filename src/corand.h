/**
 * 伪随机数生成器(PRNG)，基于：xoshiro256**
 * 生成的数范围为 [1~2^64]
 */
#ifndef _CORAND_H
#define _CORAND_H
#include <stdint.h>
#include <time.h>
#include <float.h>

/* must throw out the extra (64 - FIGS) bits */
#define shift64_FIG	(64 - DBL_MANT_DIG)
/* to scale to [0, 1), multiply by scaleFIG = 2^(-FIGS) */
#define scaleFIG	(0.5 / ((uint64_t)1 << (DBL_MANT_DIG - 1)))

typedef struct randstate {
	uint64_t s[4];
} randstate_t;

// 初始化随机数生成器
void randseed(randstate_t *state, uint64_t seed1, uint64_t seed2);

// 生成下一个随机数，范围是：[0, 2^64)
uint64_t randnext(randstate_t *state);

// 生成整型随机数
static inline int64_t randint(randstate_t *state) {
	return (int64_t)randnext(state);
}

// 生成浮点随机数，范围是：[0, 1)
static inline double randfloat(randstate_t *state) {
	uint64_t x = randnext(state);
	return (double)((x >> shift64_FIG) * scaleFIG);
}

// 生成浮点随机数，范围是：[a, b)
static inline double randfltrange(randstate_t *state, double a, double b) {
	double low = a < b ? a : b;
	double up = a < b ? b : a;
	return low + (up - low) * randfloat(state);
}

// 生成整型随机数，范围是：[a, b]
int64_t randintrange(randstate_t *state, int64_t a, int64_t b);

#endif // _CORAND_H

