#ifndef afsk_h
#define afsk_h

#include "dsp.h"
#include "pll.h"

typedef struct {
	EnvelopeDetector_struct EnvelopeDetector;
	CircularBuffer_struct Buffer1;
	FIR_struct InputFilter;
	CircularBuffer_struct Buffer2;
	FIR_struct HilbertFilter;
	FIR_struct DelayFilter;
	CMA_Equalizer_struct EQ;
	ComplexCircularBuffer_struct Buffer3;
	FIR_struct Mark;
	FIR_struct Space;
	CircularBuffer_struct Buffer4;
	FIR_struct OutputFilter;
	int SampleDelay;
} AFSKDemod_struct;

typedef struct {
	PLL_struct PLL;
	EnvelopeDetector_struct EnvelopeDetector;
	CircularBuffer_struct Buffer1;
	FIR_struct InputFilter;
	CircularBuffer_struct Buffer2;
	FIR_struct HilbertFilter;
	FIR_struct DelayFilter;
	CMA_Equalizer_struct EQ;
	FIR_struct Mark;
	FIR_struct Space;
	CircularBuffer_struct Buffer4;
	FIR_struct OutputFilter;
	int SampleDelay;
} AFSKPLLDemod_struct;

void InitAFSK(FILE *, AFSKDemod_struct *, float, float, float, float, float, float, float, int, float);
void InitAFSKPLL(FILE *, AFSKPLLDemod_struct *, float, float, float, float, float, float, float, float, float, int, float);
float DemodAFSK(FILE *, AFSKDemod_struct *, float, int);
float DemodAFSKPLL(FILE *, AFSKPLLDemod_struct *, float, int);
void InitToneCorrelator(FIR_struct *, float , float , float );

#endif