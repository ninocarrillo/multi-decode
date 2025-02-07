#include <stdint.h>

typedef struct {
    float Clock;
    float ClockStep;
    float LastSample;
    float LockRate;
    int BitIndex;
    long int DataAccumulator;
    int SyncDCD;
    int MatchDCD;
    int DCDLoad;
	int AccumulatorBitWidth;
} Data_Slicer_struct;

void InitSlice2(Data_Slicer_struct *, float, float, float);
long int Slice2(Data_Slicer_struct *, float);