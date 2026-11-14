
#include <math.h>
#include <assert.h>
#include <volconv.h>

static int sgInitialized = 0;

static float sgLinToMbTable[32];
static float sgOneOverDelta[31];
static float sgOneOverLog2;

static float LerpPreDiv(float x, float x1, float one_over_dx, float y1, float y2);
static void Init(void);
static float LinToMb(double linearVol);

float VolLinearToMillibel(float linearVol)
{
    assert(linearVol >= 0.0f && linearVol <= 1.0f);

    if (sgInitialized == 0) {
        Init();
    }

    float scaledLinear;
    int floored;

    scaledLinear = linearVol * 31.0f;
    floored = (int)scaledLinear;

    if (floored == scaledLinear) {
        return sgLinToMbTable[floored];
    } else {
        return LerpPreDiv(scaledLinear, floored, 1.0f, sgLinToMbTable[floored], sgLinToMbTable[floored + 1]);
    }
}

static float LerpPreDiv(float x, float x1, float one_over_dx, float y1, float y2)
{
    return (x - x1) * (y2 - y1) * one_over_dx + y1;
}

static void Init(void)
{
    sgOneOverLog2 = 1.0 / log(2.0);
    sgLinToMbTable[0] = -10000.0f;
    sgLinToMbTable[31] = 0.0f;

    for (int i = 1; i < 31; i++) {
        sgLinToMbTable[i] = LinToMb(float(i) * 0.032258064f);
    }

    for (i = 1; i < 32; i++) {
        sgOneOverDelta[i - 1] = 1.0f / (sgLinToMbTable[i] - sgLinToMbTable[i - 1]);
    }

    sgInitialized = 1;
}

static float LinToMb(double linearVol) {
    assert(linearVol != 0.0);
    return float(log(1.0 / linearVol)) * -1000.0f * sgOneOverLog2;
}

float VolMillibelToLinear(float millibels)
{
    assert(millibels >= -10000.0f && millibels <= 0.0f);

    if (sgInitialized == 0) {
        Init();
    }

    int iLeft = 0;
    int iRight = 31;
    while (1) {
        int diff = iRight - iLeft;
        if (diff == 1)
            break;
        int iMid = iLeft + (diff >> 1);
        if (sgLinToMbTable[iMid] > millibels)
            iRight = iMid;
        else
            iLeft = iMid;
    }

    return LerpPreDiv(millibels, sgLinToMbTable[iLeft], sgOneOverDelta[iLeft], (float) iLeft, (float) iRight) * 0.032258064f;
}
