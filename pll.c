#include "pll.h"
#include <math.h>

void InitPLL(PLL_struct *this, float set_freq, float filter_cutoff, float p_gain, float i_gain, float i_limit) {
	this->SetFrequency = set_freq;
	this->Control = 0;
	this->ProportionalGain = p_gain;
	this->IntegralGain = i_gain;
	this->IntegralLimit = i_limit;
	InitIIROrder1(&this->LoopFilter, filter_cutoff);
}

float UpdatePLL(PLL_struct *this, float sample) {

	return this->Control;
}