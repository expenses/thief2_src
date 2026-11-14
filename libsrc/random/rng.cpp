#include "rng.h"
#include "rngcong.h"
#include "rngfib.h"
#include "random.h"

#define _USE_MATH_DEFINES
#include <cmath>

#define M_PI 3.14159265358979323846

float RNG::GetFloat()
{
    long result = 0x7FFFFF & GetLong() | 0x3F800000;

    return *reinterpret_cast<float*>(&result) - 1.0;
}

float RNG::GetNorm()
{
    return cos(GetFloat() * 2.0 * M_PI) * sqrt(log(GetFloat() * -2.0));
}

long RNG::GetRange(long max)
{
    return GetLong() % max;
}

RNG* CreateRNGCongruential()
{
    return new RNGCongruential();
}

RNG* CreateRNGFibonacci()
{
    return new RNGFibonacci();
}

RNG* gRNGCongruential = NULL;
RNG* gRNGFibonacci = NULL;
