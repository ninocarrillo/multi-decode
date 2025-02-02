#define MAX_FIR_TAP_COUNT 1000
#include <stdint.h>
#include <complex.h>
#include <stdio.h>

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
	CircularBuffer_struct Buffer1;
	FIR_struct InputFilter;
	CircularBuffer_struct Buffer2;
	FIR_struct HilbertFilter;
	FIR_struct DelayFilter;
	ComplexCircularBuffer_struct Buffer3;
	FIR_struct Mark;
	FIR_struct Space;
	CircularBuffer_struct Buffer4;
	FIR_struct OutputFilter;
	int SampleDelay;
} AFSKDemod_struct;



int InitHilbert(FIR_struct *, FIR_struct *, int);
void PutCB(CircularBuffer_struct *, float);
void PutComplexCB(ComplexCircularBuffer_struct *, float complex);
void CombineFIR(FIR_struct *, FIR_struct *);
float FilterCB(CircularBuffer_struct *, FIR_struct *);
float CorrelateComplexCB(ComplexCircularBuffer_struct *, FIR_struct *);
float sinc(float);
void GenLowPassFIR(FIR_struct *, float, float, int);
void GenHighPassFIR(FIR_struct *, float, float, int);
void GenBandFIR(FIR_struct *, float, float, float, int);
int InterleaveInt16(int16_t *, int16_t *, int16_t *, int);
void InitAFSK(FILE *, AFSKDemod_struct *, float, float, float, float, float, float, float);
float DemodAFSK(FILE *, AFSKDemod_struct *, float);