#ifndef nco_h
#define nco_h
#include "lfsr.h"

#define MAX_NCO_WAVETABLE_SIZE 65536

#define DITHER_POLY 0x120001

typedef struct {
    float Wavetable[MAX_NCO_WAVETABLE_SIZE];
    LFSR_struct Dither;
    float SampleRate;
    unsigned int PAcc;
    unsigned int PAccMask;
    unsigned int DitherMask;
    int WavetableSize;
    int PAccSize;
    int TableShift;
} NCO_struct;

float GetNCOSampleFromFCW(NCO_struct *, int);
float GetNCOSampleFromFreq(NCO_struct *, float);
void InitNCO(NCO_struct *, int , int, int, float);

#endif