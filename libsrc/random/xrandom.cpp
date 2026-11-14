#define USE_RANDOM_HELPER_FUNCTIONS
#include <random.h>
#include <rng.h>

extern RNG *gRNGCongruential;
extern RNG *gRNGFibonacci;

void RandInit(long seed)
{
    gRNGCongruential = CreateRNGCongruential();
    gRNGCongruential->Seed(seed);
    gRNGFibonacci = CreateRNGFibonacci();
    gRNGFibonacci->Seed(seed);
}

void RandShutdown(void)
{
    delete gRNGCongruential;
    gRNGCongruential = NULL;
    delete gRNGFibonacci;
    gRNGFibonacci = NULL;
}

void RandSeed(long seed)
{
    gRNGFibonacci->Seed(seed);
}

long RandLong(void)
{
    return gRNGFibonacci->GetLong();
}

float RandFloat(void)
{
    return gRNGFibonacci->GetFloat();
}

float RandNorm(void)
{
    return gRNGFibonacci->GetNorm();
}

long RandRangeLong(int n)
{
    return gRNGFibonacci->GetRange(n);
}

void RandSeedQ(long seed)
{
    gRNGCongruential->Seed(seed);
}

long RandLongQ(void)
{
    return gRNGCongruential->GetLong();
}

float RandFloatQ(void)
{
    return gRNGCongruential->GetFloat();
}

float RandNormQ(void)
{
    return gRNGCongruential->GetNorm();
}

long RandRangeLongQ(int n)
{
    return gRNGCongruential->GetRange(n);
}
