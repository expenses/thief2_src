#pragma once

#include "rng.h"

class RNGCongruential : public RNG {
private:
    struct State
    {
        int Magic, Seed;
    };

public:
    RNGCongruential();
    ~RNGCongruential();

    void* GetState(long* sz);
    void SetState(void*);
    void Seed(long seed);
    long GetLong();

private:
    int m_seed;
};
