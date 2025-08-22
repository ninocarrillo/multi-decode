#ifndef pll_h
#define pll_h
#include "nco.h"
#include "iir.h"

typedef struct {
    NCO_struct NCO;
    IIR_Order1_struct LoopFilter;
    float SetFrequency;
    float Control;
    float Mixer;
    float LoopFilterOutput;
    float ProportionalGain;
    float IntegralGain;
    float Proportional;
    float Integral;
    float IntegralLimit;
} PLL_struct;


#endif