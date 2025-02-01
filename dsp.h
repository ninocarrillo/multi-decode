#define MAX_FIR_TAP_COUNT 1000
#include <stdint.h>
#include <complex.h>
typedef struct {
	float Taps[MAX_FIR_TAP_COUNT];
	float Gain;
	float SampleRate;
	int TapCount;
} FIR_struct;

typedef struct {
	float Buffer[MAX_FIR_TAP_COUNT];
	int Index;
	int Length;
} CircularBuffer_struct;

typedef struct {
	float complex Buffer[MAX_FIR_TAP_COUNT];
	int Index;
	int Length;
} ComplexCircularBuffer_struct;

typedef struct {
	FIR_struct Hilbert;
	FIR_struct InputFilter1;
	FIR_struct InputFilter2;
	
} AFSKDemod_struct;

int InitHilbert(FIR_struct *, FIR_struct *, int);
void PutCB(CircularBuffer_struct *, float);
void CombineFIR(FIR_struct *, FIR_struct *);
float FilterCB(CircularBuffer_struct *, FIR_struct *);
float CorrelateComplexCB(ComplexCircularBuffer_struct *, FIR_struct *);
float sinc(float);
void GenLowPassFIR(FIR_struct *, float, float, int);
void GenHighPassFIR(FIR_struct *, float, float, int);
void GenBandFIR(FIR_struct *, float, float, float, int);
int InterleaveInt16(int16_t *, int16_t *, int16_t *, int);