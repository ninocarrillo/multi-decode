#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <complex.h>
#include "dsp.h"
#include "log.h"

void InitEnvelopeDetector(EnvelopeDetector_struct *detector, float attack, float decay, float sustain) {
	detector->AttackRate = attack;
	detector->DecayRate = decay;
	detector->SustainPeriod = sustain;
	detector->SustainCount = 0;
	detector->Zero = 0;
	detector->Envelope = 0;
	detector->PosPeak = 0;
	detector->NegPeak = 0;
	detector->LastValue = 0;
}

float EnvelopeDetect(EnvelopeDetector_struct *detector, float signal_value) {
    // Perform peak detection, record magnitude of peaks, return filtered peak
    // value.
    
    // Determine if signal is rising or falling.
    if (signal_value > detector->LastValue) {
        // Signal is rising.
        if (signal_value >= detector->PosPeak) {
            if ((detector->PosPeak + detector->AttackRate) > signal_value) {
                detector->PosPeak = signal_value;
            } else {
                detector->PosPeak += detector->AttackRate;
            }
            detector->SustainCount = 0;
        }
    } else {
		// Signal is falling.
		if (signal_value <= detector->NegPeak) {
			if ((detector->NegPeak - detector->AttackRate) < signal_value) {
				detector->NegPeak = signal_value;
			} else {
				detector->NegPeak -= detector->AttackRate;
			}
			detector->SustainCount = 0;
		}
	}
    detector->LastValue = signal_value;
	
    // Apply a Sustain/Decay process.
    if (detector->SustainCount++ >= detector->SustainPeriod) {
        // Sustain interval has elapsed.
        // Decay the envelope.
        detector->PosPeak -= detector->DecayRate;
		detector->NegPeak += detector->DecayRate;
    }
	detector->Envelope = detector->PosPeak - detector->NegPeak;
	detector->Zero = detector->NegPeak + (detector->Envelope / 2);
    // Return the envelope.
    return detector->Envelope;
}

void ResetCMATaps(CMA_Equalizer_struct *eq) {
	for (int i = 0; i < eq->Filter.TapCount; i++) {
		eq->Filter.Taps[i] = 0;
	}
	eq->Filter.Taps[eq->Filter.TapCount / 2] = 1;
}

void InitCMAEqualizer(CMA_Equalizer_struct *eq, int tap_count, float complex mu) {
	//if (tap_count % 2) {
		// tap_count is odd, this is good.
	//} else {
		// make tap_count odd.
	//	tap_count++;
	//}

	for (int i = 0; i < tap_count; i++) {
		eq->Filter.Taps[i] = 0;
		eq->Buffer.Buffer[i] = 0;
	}
	eq->Filter.Taps[tap_count / 2] = 1;
	eq->Filter.TapCount = tap_count;
	eq->Buffer.Length = tap_count;
	eq->Buffer.Index = 0;
	eq->mu = mu/* / pow(tap_count, 2)*/;
}

float complex CMAEq(CMA_Equalizer_struct *eq, float complex sample) {
	PutComplexCB(&eq->Buffer, sample);
	return FilterComplexCB(&eq->Buffer, &eq->Filter);
}

float complex CMAEqFeedback(CMA_Equalizer_struct *eq, float complex sample) {
	PutComplexCB(&eq->Buffer, sample);
	float complex accumulator = FilterComplexCB(&eq->Buffer, &eq->Filter);
	float complex error = cabs(accumulator) - 1;
	float complex adjust = accumulator * error * eq->mu;
	int i, j;
	j = eq->Buffer.Index + 1;
	for (i = 0; i < eq->Filter.TapCount; i++) {
		eq->Filter.Taps[i] -= adjust * conj(eq->Buffer.Buffer[j]);
		j++;
		if (j < 0) {
			j += eq->Buffer.Length;
		}
		if (j >= eq->Buffer.Length) {
			j = 0;
		}
	}

	return accumulator;
}

int InitHilbert(FIR_struct *hilbert_filter, FIR_struct *delay_filter, int tap_count) {
	if (tap_count % 2) {
		// tap_count is odd, this is good.
	} else {
		// make tap_count odd.
		tap_count++;
	}
	
	// Calculate the Hilbert and Half-band filter taps
	int N = tap_count - 1;
	int delay;
	int n, i = 0;
	delay = tap_count / 2;
	for (n = -delay; n < (tap_count - delay); n++) {
		if (n % 2) {
			// n is odd
			hilbert_filter->Taps[i] = 2 / (M_PI * n);
		} else {
			// n is even
			hilbert_filter->Taps[i] = 0;
		}
		if (n == 0) {
			delay_filter->Taps[i] = 1;
		} else {
			delay_filter->Taps[i] = sin(n * M_PI / 2) / (n * M_PI);
		}
		i++;
	}
	hilbert_filter->TapCount = i;
	delay_filter->TapCount = i;
	
	// Apply a Hann window to the filters.
	for (i = 0; i < tap_count; i++) {
		float window = pow(sin(M_PI * i / N), 2);
		hilbert_filter->Taps[i] *= window;
		delay_filter->Taps[i] *= window;
	}


	// Calculate the passband gain of the half band filter (sum of the positive taps)
	delay_filter->Gain = 0;
	for (i = 0; i < tap_count; i++) {
		delay_filter->Gain += delay_filter->Taps[i];
	}
	
	// Normalize the gain of both filters.
	for (i = 0; i < tap_count; i++) {
			hilbert_filter->Taps[i] *= (1.5/delay_filter->Gain);
			delay_filter->Taps[i] *= (1/delay_filter->Gain);
	}
	delay_filter->Gain = 0;
	for (i = 0; i < tap_count; i++) {
		delay_filter->Gain += delay_filter->Taps[i];
	}
	hilbert_filter->Gain = delay_filter->Gain; 	

	delay++;
	return delay;
}

void InitCB(CircularBuffer_struct *buffer, int length) {
	buffer->Length = length;
	buffer->Index = 0;
}

void InitComplexCB(ComplexCircularBuffer_struct *buffer, int length) {
	buffer->Length = length;
	buffer->Index = 0;
}

void IncrementCB(CircularBuffer_struct *buffer) {
	buffer->Index++;
	if (buffer->Index >= buffer->Length) {
		buffer->Index = 0;
	}
}

void PutCB(CircularBuffer_struct *buffer, float value) {
	// Pre-increment circular buffer, put value.
	buffer->Index++;
	if (buffer->Index >= buffer->Length) {
		buffer->Index = 0;
	}
	buffer->Buffer[buffer->Index] = value;
	// Index points to newest value.
}

void PutComplexCB(ComplexCircularBuffer_struct *buffer, float complex value) {
	// Pre-increment circular buffer, put value.
	buffer->Index++;
	if (buffer->Index >= buffer->Length) {
		buffer->Index = 0;
	}
	buffer->Buffer[buffer->Index] = value;
	// Index points to newest value.
}

float FilterCB(CircularBuffer_struct *buffer, FIR_struct *filter) {
	int i, j;
	int n;
	float result = 0;
	// Determine limiting dimension
	if (buffer->Length > filter->TapCount) {
		n = filter->TapCount;
	} else {
		n = buffer->Length;
	}
	j = buffer->Index;
	for (i = (n-1); i >= 0; i--) {
		if (j < 0) {
			j += buffer->Length;
		}
		result += filter->Taps[i] * buffer->Buffer[j--];
	}
	return result;
}

float complex FilterComplexCB(ComplexCircularBuffer_struct *buffer, ComplexFIR_struct *filter) {
	int i, j;
	int n;
	float complex result = 0;
	// Determine limiting dimension
	if (buffer->Length > filter->TapCount) {
		n = filter->TapCount;
	} else {
		n = buffer->Length;
	}
	j = buffer->Index;
	for (i = (n-1); i >= 0; i--) {
		if (j < 0) {
			j += buffer->Length;
		}
		result += filter->Taps[i] * buffer->Buffer[j--];
	}
	return result;
}


float sinc(float value) {
	float result;
	if (value == 0) {
		result = 1;
	} else {
		result = sin(value) / value;
	}
	return result;
}

void GenLowPassFIR(FIR_struct *filter, float cutoff_freq, float sample_rate, int tap_count) {
	// Scale cutoff freq to radians / sec.
	cutoff_freq = (2 * M_PI * cutoff_freq);
	float step_size = 1 / sample_rate;
	// Calculate the starting value for time t.
	float t = -(step_size * (tap_count - 1) / 2);
	int i;
	filter->Gain = 0;
	for (i = 0; i < tap_count; i++) {
		// Calculate the discrete time impulse response of the filter. 
		filter->Taps[i] = cutoff_freq * sinc(cutoff_freq * t);
		// Apply a Hann window to the filter.
		filter->Taps[i] *= pow(sin(M_PI * i / (tap_count - 1)), 2);
		// Sum the gain of the filter.
		filter->Gain += filter->Taps[i];
		t += step_size;
	}

	// Normalize filter gain to 1.0.
	for (i = 0; i < tap_count; i++) {
		filter->Taps[i] /= filter->Gain;
	}
	// Calculate actual filter gain.
	filter->Gain = 0;
	for (i = 0; i < tap_count; i++) {
		filter->Gain += filter->Taps[i];
	}

	filter->TapCount = tap_count;
	filter->SampleRate = sample_rate;
}

void GenHighPassFIR(FIR_struct *filter, float cutoff_freq, float sample_rate, int tap_count) {
	// Check tap_count is odd, make odd if not.
	if (tap_count % 2) {
		// This is ok, do nothing.
	} else {
		// tap_count is even, add one to make odd.
		tap_count++;
	}
	// Start with a low pass filter.
	GenLowPassFIR(filter, cutoff_freq, sample_rate, tap_count);
	// Invert the taps.
	int i;
	for (i = 0; i < tap_count; i++) {
		filter->Taps[i] = -filter->Taps[i];
	}
	// Add normalized gain to the center tap.
	filter->Taps[(tap_count - 1) / 2] += filter->Gain;
}

void CombineFIR(FIR_struct *f, FIR_struct *g) {
	// Combine two FIR filters by convolving the taps.
	// The combined FIR is placed in f.
	int n, m, fi, gi;
	int M;
	int final_tap_count = f->TapCount + g->TapCount + 1;
	float h_buf[MAX_FIR_TAP_COUNT];
	float fx, hx;
	int largest_tap_count = 0;
	if (f->TapCount > g->TapCount) {
		largest_tap_count = f->TapCount;
	} else {
		largest_tap_count = g->TapCount;
	}
	// Constrain the combined FIR length.
	if (final_tap_count > MAX_FIR_TAP_COUNT) {
		final_tap_count = MAX_FIR_TAP_COUNT;
	}
	for (n = 0; n < final_tap_count; n++) {
		h_buf[n] = 0;
		for (m = 0; m < g->TapCount; m++) {
			fi = n - m;
			if ((fi >= 0) && (fi < f->TapCount)) {
				fx = f->Taps[fi];
			} else {
				fx = 0;
			}
			h_buf[n] += fx * g->Taps[m];
		}
	}
	for (n = 0; n < final_tap_count; n++) {
		f->Taps[n] = h_buf[n];
	}
	f->Gain *= g->Gain;
	f->TapCount = final_tap_count;
}

void GenBandFIR(FIR_struct *filter, float freq1, float freq2, float sample_rate, int tap_count) {
	FIR_struct hpf;
	GenLowPassFIR(filter, freq2, sample_rate, tap_count / 2);
	GenHighPassFIR(&hpf, freq1, sample_rate, tap_count / 2);
	CombineFIR(filter, &hpf);
}

int InterleaveInt16(int16_t *output, int16_t *left_data, int16_t *right_data, int count) {
	int i, j;
	j = 0;
	for (i = 0; i < count; i++) {
		output[j++] = left_data[i];
		output[j++] = right_data[i];
	}
	return j;
}

float CorrelateComplexCB(ComplexCircularBuffer_struct *buffer, FIR_struct *filter) {
	int i, j;
	int n;
	float complex result;
	// Determine limiting dimension
	if (buffer->Length > filter->TapCount) {
		n = filter->TapCount;
	} else {
		n = buffer->Length;
	}
	result = 0+0*I;
	j = buffer->Index;
	for (i = (n-1); i >= 0; i--) {
		if (j < 0) {
			j += buffer->Length;
		}
		result += filter->Taps[i] * buffer->Buffer[j--];
	}
	return cabs(result);
}

void InitToneCorrelator(FIR_struct *correlator, float freq, float sample_rate, float symbol_rate) {
	int tap_count = 1.5 * sample_rate / symbol_rate;
	correlator->SampleRate = sample_rate;
	correlator->Gain = 1;
	correlator->TapCount = tap_count;
	for (int i = 0; i < tap_count; i++) {
		float t = (M_PI * freq * 2 * i) / (sample_rate);
		correlator->Taps[i] = cos(t);
	}
	
	int N = tap_count - 1;
	// Apply a Hann window to the filter.
	for (int i = 0; i < tap_count; i++) {
		float window = pow(sin(M_PI * i / N), 2);
		correlator->Taps[i] *= window;
	}
	
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
		result = CMAEqFeedback(&demod->EQ, result);
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

	// Place result in buffer.
	PutCB(&demod->Buffer4, creal(1024 * result));

	// Apply output filter.
	result = FilterCB(&demod->Buffer4, &demod->OutputFilter);

	return creal(result);
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
	//InitCMAEqualizer(&demod->EQ, 9, mu);
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

	// Create the output Lowpass filter spanning 5 symbols.
	int output_tap_count = 5 * sample_rate / symbol_rate;
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
	InitComplexCB(&demod->Buffer3, demod->Mark.TapCount);
	InitCB(&demod->Buffer4, demod->OutputFilter.TapCount);

	// Calculate the sample delay.
	demod->SampleDelay = 0;
	demod->SampleDelay += demod->InputFilter.TapCount;
	demod->SampleDelay += demod->HilbertFilter.TapCount;
	demod->SampleDelay += demod->Mark.TapCount;
	demod->SampleDelay += demod->OutputFilter.TapCount;

}