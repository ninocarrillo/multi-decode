#ifndef ted_h
#define ted_h

#include <stdint.h>
#include "dsp.h"

typedef struct {
    float Clock;
    float ClockStep;
    float LastSample;
    float LockRate;
	float Interval;
	int BitsPerSymbol;
    int BitIndex;
	int Symbol;
	int SymbolTap;
	int BitsLeftInSymbol;
    long int DataAccumulator;
    int SyncDCD;
    int MatchDCD;
    int DCDLoad;
	int AccumulatorBitWidth;
} Data_Slicer_struct;

typedef struct {
    float TimingError;
    float SampleB;
    float SampleC;
    long int DataAccumulator;
    int BitIndex;
    int AccumulatorBitWidth;
    int SyncDCD;
    int MatchDCD;
    int Decimation;
    int DecimationIndex;
    int DCDLoad;
    float RegeneratedClock;
    float RegeneratedClockRate;
    unsigned Timer :1;
} Gardner_TED_struct;

void InitSlice2(Data_Slicer_struct *, float, float, float);
long int Slice2(Data_Slicer_struct *, float);
long int Slice2Eq(Data_Slicer_struct *, CMA_Equalizer_struct *, float);
void InitSliceN(Data_Slicer_struct *, float , float , float , int );
long int SliceN(Data_Slicer_struct *, float);
long int GardnerLinear(Gardner_TED_struct *, float );
void InitGardnerLinear(Gardner_TED_struct *, float , float);
#endif