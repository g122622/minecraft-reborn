#include "Xoroshiro128ppRandom.hpp"

namespace mr::math {

Xoroshiro128ppRandom::Xoroshiro128ppRandom(u64 seed) {
    setSeed(seed);
}

void Xoroshiro128ppRandom::setSeed(u64 seed) {
    // 使用 SplitMix64 扩展种子
    u64 state = seed;
    m_state[0] = splitMix64(state);
    m_state[1] = splitMix64(state);

    // 确保状态不全为零
    if (m_state[0] == 0 && m_state[1] == 0) {
        m_state[0] = 1;
    }

    m_hasGaussian = false;
}

u64 Xoroshiro128ppRandom::nextU64() {
    const u64 s0 = m_state[0];
    u64 s1 = m_state[1];

    // xoroshiro128++ 核心算法
    const u64 result = rotl(s0 + s1, 17) + s0;

    // 更新状态
    s1 ^= s0;
    m_state[0] = rotl(s0, 49) ^ s1 ^ (s1 << 21);
    m_state[1] = rotl(s1, 28);

    return result;
}

void Xoroshiro128ppRandom::skip(u64 count) {
    // xoroshiro128++ 的快速跳转
    // 使用预计算的跳转多项式
    // 参考 http://xoroshiro.di.unimi.it/xoroshiro128plusplus.c

    static const u64 JUMP[] = {
        0x2bd7a6a6e99c2ddcULL,
        0x0992ccaf6a6fca05ULL
    };

    u64 s0 = 0;
    u64 s1 = 0;

    for (size_t i = 0; i < sizeof(JUMP) / sizeof(JUMP[0]); ++i) {
        for (int b = 0; b < 64; ++b) {
            if (JUMP[i] & (1ULL << b)) {
                s0 ^= m_state[0];
                s1 ^= m_state[1];
            }
            nextU64();
        }
    }

    m_state[0] = s0;
    m_state[1] = s1;
}

u64 Xoroshiro128ppRandom::splitMix64(u64& state) {
    state += 0x9e3779b97f4a7c15ULL;
    u64 z = state;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

} // namespace mr::math
