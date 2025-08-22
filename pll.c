#include "pll.h"
#include <math.h>

void InitPLL(PLL_struct *this, float sample_rate, float set_freq, float filter_cutoff, float p_gain, float i_gain, float i_limit) {
	this->SetFrequency = set_freq;
	this->Control = 0;
	this->ProportionalGain = p_gain;
	this->IntegralGain = i_gain;
	this->IntegralLimit = i_limit;
	InitIIROrder1(&this->LoopFilter, sample_rate, filter_cutoff);
	InitNCO(&this->NCO, 16, 31, 7, sample_rate);
}

float UpdatePLL(PLL_struct *this, float sample) {
	this->Mixer = GetNCOSampleFromFreq(&this->NCO, this->SetFrequency + this->Control) * sample;
	this->LoopFilterOutput = UpdateIIROrder1(&this->LoopFilter, this->Mixer);
	this->Integral += this->IntegralGain * this->LoopFilterOutput;
	if (this->Integral > this->IntegralLimit) {
		this->Integral = this->IntegralLimit;
	} else if (this->Integral < -this->IntegralLimit) {
		this->Integral = -this->IntegralLimit;
	}
	this->Proportional = this->ProportionalGain * this->LoopFilterOutput;
	this->Control = this->Proportional + this->Integral;
	return this->Control;
}