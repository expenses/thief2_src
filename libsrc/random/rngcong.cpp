

#include "rngcong.h"

#include <cmath>
#include <lgassert.h>

static const int CongruentialMagic = 23479589;

RNGCongruential::RNGCongruential()
{
    m_seed = 0;
}

RNGCongruential::~RNGCongruential() { }

void* RNGCongruential::GetState(long* sz)
{
    *sz = sizeof(State);

    State* result = new State();

    result->Magic = CongruentialMagic;
    result->Seed = m_seed;

    return result;
}

void RNGCongruential::SetState(void* rawState)
{
    State* state = reinterpret_cast<State*>(rawState);

    AssertMsg(state->Magic == CongruentialMagic, "Invalid state for RNGCongruential::SetState");

    m_seed = state->Seed;
}

void RNGCongruential::Seed(long seed)
{
    m_seed = seed;
}

long RNGCongruential::GetLong()
{
    m_seed = 1664525 * m_seed + 12345;
    return ulong(m_seed) >> 1;
}
