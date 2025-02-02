#include <stdint.h>

typedef struct {
    float Clock;
    float ClockStep;
    float LastSample;
    float LockRate;
    int BitIndex;
    uint32_t DataAccumulator;
    int SyncDCD;
    int MatchDCD;
} Data_Slicer_struct;

void InitSlice2(Data_Slicer_struct *, float, float, float);
int Slice2(Data_Slicer_struct *, float);