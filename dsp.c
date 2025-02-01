#include "dsp.h"
#include <math.h>
#include <stdio.h>
#include <stdint.h>

int InitHilbert(FIR_struct *hilbert_filter, FIR_struct *delay_filter, int tap_count) {
	if (tap_count % 2) {
		// tap_count is odd, this is good.
	} else {
		// make tap_count odd.
		tap_count++;
	}
	
	// Calculate the Hilbert filter taps
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
		i++;
	}
	hilbert_filter->TapCount = i;
	
	// Apply a Hann window to the filter.
	for (i = 0; i < tap_count; i++) {
		hilbert_filter->Taps[i] *= pow(sin(M_PI * i / N), 2);
	}
	delay++;
	for (i = 0; i < delay; i++) {
		delay_filter->Taps[i] = 0;
	}
	delay_filter->Taps[0] = 1;
	delay_filter->TapCount = delay;
	delay_filter->Gain = 1;
	return delay;
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