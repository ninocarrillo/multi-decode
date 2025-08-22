#include "nco.h"
#include <math.h>

void InitNCO(NCO_struct *this, int wavetable_bits, int pacc_bits, float samp_rate) {
	int i;
	int wavetable_size = 1 << wavetable_bits;
	this->PAccSize = 1 << pacc_bits;
	this->PAccMask = this->PAccSize - 1;
	this->TableShift = pacc_bits - wavetable_bits;
	this->SampleRate = samp_rate;
	double x;
	for (i = 0; i < wavetable_size; i++) {
		x = 2 * (double)i * M_PI / (double)wavetable_size;
		this->Wavetable[i] = sin(x);
	}
}

float GetNCOSampleFromFCW(NCO_struct *this, int fcw) {
    this->PAcc += fcw;
    this->PAcc &= this->PAccMask;
    return this->Wavetable[this->PAcc>>this->TableShift];
}

float GetNCOSampleFromFreq(NCO_struct *this, float freq) {
	int fcw = freq * (float)this->PAccSize / (float)this->SampleRate;
	this->PAcc += fcw;
	this->PAcc &= this->PAccMask;
    return this->Wavetable[this->PAcc>>this->TableShift];
}