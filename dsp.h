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
	float complex Taps[MAX_FIR_TAP_COUNT];
	float Gain;
	float SampleRate;
	int TapCount;
} ComplexFIR_struct;

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
	ComplexFIR_struct Filter;
	ComplexCircularBuffer_struct Buffer;
	complex float mu;
} CMA_Equalizer_struct;

typedef struct {
    float LastValue;
	float PosPeak;
	float NegPeak;
	float Zero;
    float Envelope;
    float AttackRate;
    float DecayRate;
    float SustainPeriod;
    int SustainCount;
} EnvelopeDetector_struct;

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



int InitHilbert(FIR_struct *, FIR_struct *, int);
void PutCB(CircularBuffer_struct *, float);
void PutComplexCB(ComplexCircularBuffer_struct *, float complex);
void CombineFIR(FIR_struct *, FIR_struct *);
float FilterCB(CircularBuffer_struct *, FIR_struct *);
float complex FilterComplexCB(ComplexCircularBuffer_struct *, ComplexFIR_struct *);
float CorrelateComplexCB(ComplexCircularBuffer_struct *, FIR_struct *);
float sinc(float);
void GenLowPassFIR(FIR_struct *, float, float, int);
void GenHighPassFIR(FIR_struct *, float, float, int);
void GenBandFIR(FIR_struct *, float, float, float, int);
int InterleaveInt16(int16_t *, int16_t *, int16_t *, int);
void InitAFSK(FILE *, AFSKDemod_struct *, float, float, float, float, float, float, float, float);
float DemodAFSK(FILE *, AFSKDemod_struct *, float, int);
float complex CMAEqFeedback(CMA_Equalizer_struct *, float complex);
float complex CMAEq(CMA_Equalizer_struct *, float complex);
void InitCMAEqualizer(CMA_Equalizer_struct *, int, float complex);
float EnvelopeDetect(EnvelopeDetector_struct *, float);
void InitEnvelopeDetector(EnvelopeDetector_struct *, float, float, float);