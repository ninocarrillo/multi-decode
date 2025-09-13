#include <math.h>
#include "afsk.h"
#include "log.h"
#include "dsp.h"
#include "pll.h"


void InitToneCorrelator(FIR_struct *correlator, float freq, float sample_rate, float symbol_rate) {
	//int tap_count = 1.5 * sample_rate / symbol_rate;
	int tap_count = 1.0 * sample_rate / symbol_rate;
	correlator->SampleRate = sample_rate;
	correlator->Gain = 1;
	correlator->TapCount = tap_count;
	for (int i = 0; i < tap_count; i++) {
		float t = (M_PI * freq * 2 * i) / (sample_rate);
		correlator->Taps[i] = cos(t);
	}
	
	//int N = tap_count - 1;
	// Apply a Hann window to the filter.
	//for (int i = 0; i < tap_count; i++) {
	//	float window = pow(sin(M_PI * i / N), 2);
	//	correlator->Taps[i] *= window;
	//}
	
	// Normalize autocorrelation
	// First, calculate the autocorrelation
	correlator->PreAuto = 0;
	for (int i = 0; i < tap_count; i++) {
		correlator->PreAuto += pow(correlator->Taps[i],2);
	}
	for (int i = 0; i < tap_count; i++) {
		correlator->Taps[i] /= sqrt(correlator->PreAuto);
	}
	correlator->Auto = 0;
	for (int i = 0; i < tap_count; i++) {
		correlator->Auto += pow(correlator->Taps[i],2);
	}
}

float DemodAFSK(FILE *logfile, AFSKDemod_struct *demod, float sample, int carrier_detect) {

	// Place sample in circular buffer.
	PutCB(&demod->Buffer1, sample);

	// Apply input filter.
	float complex result = FilterCB(&demod->Buffer1, &demod->InputFilter);

	// Place filtered sample in circular buffer.
	PutCB(&demod->Buffer2, creal(result));

	// Create quadrature signal.
	result = FilterCB(&demod->Buffer2, &demod->DelayFilter) + FilterCB(&demod->Buffer2, &demod->HilbertFilter) * I;
	
	// Equalize.
	if (carrier_detect > 0) {
		result = CMAEqFeedback(&demod->EQ, result, 1);
	} else {
		result = CMAEq(&demod->EQ, result);
	}

	// Place quadrature signal in complex circular buffer.
	PutComplexCB(&demod->Buffer3, result);

	// Apply the mark correlator.
	float mark = CorrelateComplexCB(&demod->Buffer3, &demod->Mark);

	// Apply the space correlator.
	float space = CorrelateComplexCB(&demod->Buffer3, &demod->Space);

	result = mark - space;

	return result*4096;
}

void InitAFSK(FILE *logfile, AFSKDemod_struct *demod, float sample_rate, float low_cut, float high_cut, float tone1, float tone2, float symbol_rate, float output_cut, int cma_span, float cma_mu) {
	
	LogNewline(logfile);
	LogString(logfile, "Initializing AFSK Demodulator.");

	// Create the input Bandpass filter spanning 7 milliseconds of input samples.
	int input_tap_count = 0.007 * sample_rate;
	GenBandFIR(&demod->InputFilter, low_cut, high_cut, sample_rate, input_tap_count);
	LogNewline(logfile);
	LogString(logfile, "Input filter tap count: ");
	LogInt(logfile, demod->InputFilter.TapCount);
	LogNewline(logfile);
	LogString(logfile, "Taps: ");
	for (int i = 0; i < demod->InputFilter.TapCount; i++) {
		LogFloat(logfile, demod->InputFilter.Taps[i]);
		LogString(logfile, ",");
	}

	// Create a Hilbert transform filter.
	int hilbert_tap_count = 0.00125 * sample_rate;
	InitHilbert(&demod->HilbertFilter, &demod->DelayFilter, hilbert_tap_count);
	LogNewline(logfile);
	LogString(logfile, "Hilbert tap count: ");
	LogInt(logfile, demod->HilbertFilter.TapCount);
	LogNewline(logfile);
	LogString(logfile, "Taps: ");
	for (int i = 0; i < demod->HilbertFilter.TapCount; i++) {
		LogFloat(logfile, demod->HilbertFilter.Taps[i]);
		LogString(logfile, ",");
	}
	LogNewline(logfile);
	LogString(logfile, "Delay tap count: ");
	LogInt(logfile, demod->DelayFilter.TapCount);
	LogNewline(logfile);
	LogString(logfile, "Taps: ");
	for (int i = 0; i < demod->DelayFilter.TapCount; i++) {
		LogFloat(logfile, demod->DelayFilter.Taps[i]);
		LogString(logfile, ",");
	}

	InitCMAEqualizer(&demod->EQ, cma_span, cma_mu);
	LogNewline(logfile);
	LogString(logfile, "CMA Equalizer tap count: ");
	LogInt(logfile, demod->EQ.Filter.TapCount);
	LogNewline(logfile);
	LogString(logfile, "Taps: ");
	for (int i = 0; i < demod->EQ.Filter.TapCount; i++) {
		LogFloat(logfile, demod->EQ.Filter.Taps[i]);
		LogString(logfile, ",");
	}
	
	// Generate mark correlator taps spanning 1 symbol.
	InitToneCorrelator(&demod->Mark, tone1, sample_rate, symbol_rate);
	LogNewline(logfile);
	LogString(logfile, "Mark Correlator Tap Count: ");
	LogInt(logfile, demod->Mark.TapCount);
	LogNewline(logfile);
	LogString(logfile, "Uncorrected Autocorrelation: ");
	LogFloat(logfile, demod->Mark.PreAuto);
	LogNewline(logfile);
	LogString(logfile, "Corrected Autocorrelation: ");
	LogFloat(logfile, demod->Mark.Auto);
	LogNewline(logfile);
	LogString(logfile, "Taps: ");
	for (int i = 0; i < demod->Mark.TapCount; i++) {
		LogFloat(logfile, demod->Mark.Taps[i]);
		LogString(logfile, ",");
	}	

	// Generate space correlator taps spanning 1 symbol.
	InitToneCorrelator(&demod->Space, tone2, sample_rate, symbol_rate);
	LogNewline(logfile);
	LogString(logfile, "Space Correlator Tap Count: ");
	LogNewline(logfile);
	LogString(logfile, "Uncorrected Autocorrelation: ");
	LogFloat(logfile, demod->Space.PreAuto);
	LogNewline(logfile);
	LogString(logfile, "Corrected Autocorrelation: ");
	LogFloat(logfile, demod->Space.Auto);
	LogNewline(logfile);
	LogInt(logfile, demod->Space.TapCount);
	LogNewline(logfile);
	LogString(logfile, "Taps: ");
	for (int i = 0; i < demod->Space.TapCount; i++) {
		LogFloat(logfile, demod->Space.Taps[i]);
		LogString(logfile, ",");
	}


	// Initialize buffers.
	InitCB(&demod->Buffer1, demod->InputFilter.TapCount);
	InitCB(&demod->Buffer2, demod->HilbertFilter.TapCount);
	InitComplexCB(&demod->Buffer3, demod->Mark.TapCount);

	// Calculate the sample delay.
	demod->SampleDelay = 0;
	demod->SampleDelay += demod->InputFilter.TapCount;
	demod->SampleDelay += demod->HilbertFilter.TapCount;
	demod->SampleDelay += demod->Mark.TapCount;

}
void InitAFSKPLL(FILE *logfile, AFSKPLLDemod_struct *demod, float sample_rate, float low_cut, float high_cut, float pll_set_freq, float pll_loop_cutoff, float pll_p_gain, float pll_i_gain, float pll_i_limit, float output_cut, int cma_span, float cma_mu) {
	
	LogNewline(logfile);
	LogString(logfile, "Initializing AFSK PLL Demodulator.");
	
	InitEnvelopeDetector(&demod->EnvelopeDetector, sample_rate, /*attack*/ 500, /*sustain*/ 0, /*decay*/2.5); 

	// Create the input Bandpass filter spanning 7 milliseconds of input samples.
	int input_tap_count = 0.007 * sample_rate;
	GenBandFIR(&demod->InputFilter, low_cut, high_cut, sample_rate, input_tap_count);
	LogNewline(logfile);
	LogString(logfile, "Input filter tap count: ");
	LogInt(logfile, demod->InputFilter.TapCount);
	LogNewline(logfile);
	LogString(logfile, "Taps: ");
	for (int i = 0; i < demod->InputFilter.TapCount; i++) {
		LogFloat(logfile, demod->InputFilter.Taps[i]);
		LogString(logfile, ",");
	}

	// Create a Hilbert transform filter spanning 3 milliseconds of input samples.
	int hilbert_tap_count = 0.003 * sample_rate;
	InitHilbert(&demod->HilbertFilter, &demod->DelayFilter, hilbert_tap_count);
	LogNewline(logfile);
	LogString(logfile, "Hilbert tap count: ");
	LogInt(logfile, demod->HilbertFilter.TapCount);
	LogNewline(logfile);
	LogString(logfile, "Taps: ");
	for (int i = 0; i < demod->HilbertFilter.TapCount; i++) {
		LogFloat(logfile, demod->HilbertFilter.Taps[i]);
		LogString(logfile, ",");
	}
	LogNewline(logfile);
	LogString(logfile, "Delay tap count: ");
	LogInt(logfile, demod->DelayFilter.TapCount);
	LogNewline(logfile);
	LogString(logfile, "Taps: ");
	for (int i = 0; i < demod->DelayFilter.TapCount; i++) {
		LogFloat(logfile, demod->DelayFilter.Taps[i]);
		LogString(logfile, ",");
	}

	InitCMAEqualizer(&demod->EQ, cma_span, cma_mu);
	LogNewline(logfile);
	LogString(logfile, "CMA Equalizer tap count: ");
	LogInt(logfile, demod->EQ.Filter.TapCount);
	LogNewline(logfile);
	LogString(logfile, "Taps: ");
	for (int i = 0; i < demod->EQ.Filter.TapCount; i++) {
		LogFloat(logfile, demod->EQ.Filter.Taps[i]);
		LogString(logfile, ",");
	}
	
	// Generate PLL.
	InitPLL(&demod->PLL, sample_rate, pll_set_freq, pll_loop_cutoff, pll_p_gain, pll_i_gain, pll_i_limit);
	LogNewline(logfile);
	LogString(logfile, "Generated PLL.");


	// Create the output Lowpass filter spanning 3 milliseconds.
	int output_tap_count = 0.003 * sample_rate;
	GenLowPassFIR(&demod->OutputFilter, output_cut, sample_rate, output_tap_count);
	LogNewline(logfile);
	LogString(logfile, "Output filter tap count: ");
	LogInt(logfile, demod->OutputFilter.TapCount);
	LogNewline(logfile);
	LogString(logfile, "Taps: ");
	for (int i = 0; i < demod->OutputFilter.TapCount; i++) {
		LogFloat(logfile, demod->OutputFilter.Taps[i]);
		LogString(logfile, ",");
	}

	// Initialize buffers.
	InitCB(&demod->Buffer1, demod->InputFilter.TapCount);
	InitCB(&demod->Buffer2, demod->HilbertFilter.TapCount);
	InitCB(&demod->Buffer4, demod->OutputFilter.TapCount);

	// Calculate the sample delay.
	demod->SampleDelay = 0;
	demod->SampleDelay += demod->InputFilter.TapCount;
	demod->SampleDelay += demod->HilbertFilter.TapCount;
	demod->SampleDelay += demod->OutputFilter.TapCount;

}

float DemodAFSKPLL(FILE *logfile, AFSKPLLDemod_struct *demod, float sample, int carrier_detect) {

	// Place sample in circular buffer.
	PutCB(&demod->Buffer1, sample);

	// Apply input filter.
	float complex result = FilterCB(&demod->Buffer1, &demod->InputFilter);

	// Place filtered sample in circular buffer.
	PutCB(&demod->Buffer2, creal(result));

	// Create quadrature signal.
	result = FilterCB(&demod->Buffer2, &demod->DelayFilter) + FilterCB(&demod->Buffer2, &demod->HilbertFilter) * I;
	
	// Equalize.
	if (carrier_detect > 0) {
		result = CMAEqFeedback(&demod->EQ, result, 1);
		//result = CMAEqFeedbackNorm(&demod->EQ, result, 1);
	} else {
		//ResetCMATaps(&demod->EQ);
		result = CMAEq(&demod->EQ, result);
	}
	// Apply AGC
	float envelope = EnvelopeDetect(&demod->EnvelopeDetector, result);
	if (envelope != 0) {
		result = result / envelope;
	}

	// Apply PLL to real part
	result = UpdatePLL(&demod->PLL, creal(result));
	
	result = -demod->PLL.Proportional;

	// Place result in buffer.
	PutCB(&demod->Buffer4, creal(result));

	// Apply output filter.
	result = FilterCB(&demod->Buffer4, &demod->OutputFilter);
	

	return creal(result);
}

void InitAFSKQuad(FILE *logfile, AFSKQuadDemod_struct *demod, float sample_rate, float low_cut, float high_cut, float output_cut, int cma_span, float cma_mu) {
	
	LogNewline(logfile);
	LogString(logfile, "Initializing AFSK Quadrature Demodulator.");
	
	InitEnvelopeDetector(&demod->EnvelopeDetector, sample_rate, /*attack*/ 500, /*sustain*/ 0, /*decay*/2.5); 

	// Create the input Bandpass filter spanning 7 milliseconds of input samples.
	int input_tap_count = 0.007 * sample_rate;
	GenBandFIR(&demod->InputFilter, low_cut, high_cut, sample_rate, input_tap_count);
	LogNewline(logfile);
	LogString(logfile, "Input filter tap count: ");
	LogInt(logfile, demod->InputFilter.TapCount);
	LogNewline(logfile);
	LogString(logfile, "Taps: ");
	for (int i = 0; i < demod->InputFilter.TapCount; i++) {
		LogFloat(logfile, demod->InputFilter.Taps[i]);
		LogString(logfile, ",");
	}

	// Create a Hilbert transform filter spanning 3 milliseconds of input samples.
	int hilbert_tap_count = 0.003 * sample_rate;
	InitHilbert(&demod->HilbertFilter, &demod->DelayFilter, hilbert_tap_count);
	LogNewline(logfile);
	LogString(logfile, "Hilbert tap count: ");
	LogInt(logfile, demod->HilbertFilter.TapCount);
	LogNewline(logfile);
	LogString(logfile, "Taps: ");
	for (int i = 0; i < demod->HilbertFilter.TapCount; i++) {
		LogFloat(logfile, demod->HilbertFilter.Taps[i]);
		LogString(logfile, ",");
	}
	LogNewline(logfile);
	LogString(logfile, "Delay tap count: ");
	LogInt(logfile, demod->DelayFilter.TapCount);
	LogNewline(logfile);
	LogString(logfile, "Taps: ");
	for (int i = 0; i < demod->DelayFilter.TapCount; i++) {
		LogFloat(logfile, demod->DelayFilter.Taps[i]);
		LogString(logfile, ",");
	}

	InitCMAEqualizer(&demod->EQ, cma_span, cma_mu);
	LogNewline(logfile);
	LogString(logfile, "CMA Equalizer tap count: ");
	LogInt(logfile, demod->EQ.Filter.TapCount);
	LogNewline(logfile);
	LogString(logfile, "Taps: ");
	for (int i = 0; i < demod->EQ.Filter.TapCount; i++) {
		LogFloat(logfile, demod->EQ.Filter.Taps[i]);
		LogString(logfile, ",");
	}


	// Create the output Lowpass filter spanning 3 milliseconds.
	int output_tap_count = 0.003 * sample_rate;
	GenLowPassFIR(&demod->OutputFilter, output_cut, sample_rate, output_tap_count);
	LogNewline(logfile);
	LogString(logfile, "Output filter tap count: ");
	LogInt(logfile, demod->OutputFilter.TapCount);
	LogNewline(logfile);
	LogString(logfile, "Taps: ");
	for (int i = 0; i < demod->OutputFilter.TapCount; i++) {
		LogFloat(logfile, demod->OutputFilter.Taps[i]);
		LogString(logfile, ",");
	}

	// Initialize buffers.
	InitCB(&demod->Buffer1, demod->InputFilter.TapCount);
	InitCB(&demod->Buffer2, demod->HilbertFilter.TapCount);
	InitCB(&demod->Buffer4, demod->OutputFilter.TapCount);

	// Calculate the sample delay.
	demod->SampleDelay = 0;
	demod->SampleDelay += demod->InputFilter.TapCount;
	demod->SampleDelay += demod->HilbertFilter.TapCount;
	demod->SampleDelay += demod->OutputFilter.TapCount;

}

float DemodAFSKQuad(FILE *logfile, AFSKQuadDemod_struct *demod, float sample, int carrier_detect) {

	// Place sample in circular buffer.
	PutCB(&demod->Buffer1, sample);

	// Apply input filter.
	float complex complex_signal = FilterCB(&demod->Buffer1, &demod->InputFilter);

	// Place filtered sample in circular buffer.
	PutCB(&demod->Buffer2, creal(complex_signal));

	// Create quadrature signal.
	complex_signal = FilterCB(&demod->Buffer2, &demod->DelayFilter) + FilterCB(&demod->Buffer2, &demod->HilbertFilter) * I;
	
	// Equalize.
	if (carrier_detect > 0) {
		complex_signal = CMAEqFeedback(&demod->EQ, complex_signal, 1);
	} else {
		complex_signal = CMAEq(&demod->EQ, complex_signal);
	}

	// Apply AGC
	//float envelope = EnvelopeDetect(&demod->EnvelopeDetector, creal(complex_signal));
	//if (envelope != 0) {
	//	complex_signal = (creal(complex_signal) / envelope) + (cimag(complex_signal) / envelope)*I ;
	//}

	float i, q;
	if (creal(complex_signal) > 0) {
		i = 1;
	} else {
		i = -1;
	}
	if (cimag(complex_signal) > 0) {
		q = 1;
	} else {
		q = -1;
	}

	// Apply FM demodulation

	float result = (q - demod->Q2) * demod->I1;
	result -= ((i - demod->I2) * demod->Q1);


	// Place result in buffer.
	PutCB(&demod->Buffer4, result);

	demod->I2 = demod->I1;
	demod->I1 = i;
	demod->Q2 = demod->Q1;
	demod->Q1 = q;

	float gain = 4000;
	// Apply output filter.
	result = (FilterCB(&demod->Buffer4, &demod->OutputFilter) * gain) + (1.895 * gain);
	

	return creal(result);
}
