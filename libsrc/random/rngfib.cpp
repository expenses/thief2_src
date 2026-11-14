


#include "rngfib.h"

#include <memory>
#include <cmath>
#include <string.h>

#include <lgassert.h>

static const int CongruentialMagic = 23479589;

RNGFibonacci::RNGFibonacci()
{
    m_pSeed1 = m_seeds;
    m_pSeed2 = m_pSeed1 + 31;
    memset(m_seeds, 0, sizeof(m_seeds));
}

RNGFibonacci::~RNGFibonacci() { }

void* RNGFibonacci::GetState(long* sz)
{
    *sz = sizeof(State);
    State* state = new State();

    state->Seed1 = 21796;
    state->Seed2 = m_pSeed1 - m_seeds;

    memcpy(state->Seeds, m_seeds, sizeof(m_seeds));

    return state;
}

void RNGFibonacci::SetState(void* rawState)
{
    State* state = reinterpret_cast<State*>(rawState);

    AssertMsg(state->Seed1 == 21796 && state->Seed2 < 55, "Invalid state for RNGFibonacci::SetState");

    m_pSeed1 = m_seeds + state->Seed2;
    m_pSeed2 = m_pSeed1 + 31;

    if (m_pSeed2 - m_seeds > 55)
        m_pSeed2 -= 55;

    memcpy(m_seeds, state->Seeds, sizeof(m_seeds));
}

void RNGFibonacci::Seed(long seed)
{
    RNG* pCong = CreateRNGCongruential();
    pCong->Seed(seed);

    m_seeds[0] = 0x7FFFFFFF;
    for (int i = 1; i < 55; ++i)
        m_seeds[i] = pCong->GetLong();

    for (int j = 0; j < 1000; ++j)
        GetLong();

    delete pCong;
}

long RNGFibonacci::GetLong()
{
    if (++m_pSeed1 >= m_seeds + 55)
        m_pSeed1 = m_seeds;

    if (++m_pSeed2 >= m_seeds + 55)
        m_pSeed2 = m_seeds;

    *m_pSeed1 ^= *m_pSeed2;

    return *m_pSeed1;
}
