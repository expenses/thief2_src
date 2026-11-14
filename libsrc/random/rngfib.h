#pragma once

#include "rng.h"

class RNGFibonacci : public RNG {
private:
    struct State
    {
        int Seed1, Seed2, Seeds[55];
    };

public:
    RNGFibonacci();
    ~RNGFibonacci();

    void* GetState(long* sz);
    void SetState(void*);
    void Seed(long seed);
    long GetLong();

private:
    int* m_pSeed1;
    int* m_pSeed2;
    int m_seeds[55];
};
