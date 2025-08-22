#ifndef nco_h
#define nco_h

#define MAX_NCO_WAVETABLE_SIZE 32768

typedef struct {
    float Wavetable[MAX_NCO_WAVETABLE_SIZE];
    float SampleRate;
    unsigned int PAcc;
    unsigned int PAccMask;
    int WavetableSize;
    int PAccSize;
    int TableShift;
} NCO_struct;

float GetNCOSampleFromFCW(NCO_struct *, int);
float GetNCOSampleFromFreq(NCO_struct *, float);
void InitNCO(NCO_struct *, int , int, float);

#endif