/**
 * 伪随机数生成器(PRNG)，基于：xoshiro256**
 * 生成的数范围为 [1~2^64]
 */
#ifndef _CORAND_H
#define _CORAND_H
#include <stdint.h>
#include <time.h>

typedef struct randstate {
	uint64_t s[4];
} randstate_t;

// 初始化随机数生成器
void randinit(randstate_t *state, uint64_t seed1, uint64_t seed2);

// 生成下一个随机数，范围是：[0, 2^64)
uint64_t randnext(randstate_t *state);

#endif // _CORAND_H

