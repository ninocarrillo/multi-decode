#ifndef ted_h
#define ted_h

#include <stdint.h>
#include "dsp.h"
#include "iir.h"

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
	IIR_Order1_struct LoopFilter;
	long int DataAccumulator;
	float SampleFractionalTarget; /* constrain 0.0 to 1.0 */
	float I1;
	float I2;
	float S1;
	int AccumulatorBitWidth;
	int SampleBaseTarget; /* constrain 0 to oversample - 1 */
	int ZeroBaseTarget; /* constrain 0 to oversample - 1 */
	int SampleIndex;
	int Oversample;
	int BitIndex;
	int MatchDCD;
	int DCDLoad;
} Gardner_TED_struct;

void InitSlice2(Data_Slicer_struct *, float, float, float);
long int Slice2(Data_Slicer_struct *, float);
long int Slice2Eq(Data_Slicer_struct *, CMA_Equalizer_struct *, float);
void InitSliceN(Data_Slicer_struct *, float , float , float , int );
long int SliceN(Data_Slicer_struct *, float);
long int GardnerLinear(Gardner_TED_struct *, float );
void InitGardnerLinear(Gardner_TED_struct *, int , int, float);
#endif