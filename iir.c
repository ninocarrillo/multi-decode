#include "iir.h"
#include "math.h"

void InitIIROrder1(IIR_Order1_struct *iir, float sample_rate, float cutoff_freq) {
	// Convert cutoff freq to radians/sec
	float radian_cutoff = 2 * M_PI * cutoff_freq;
	// prewarp cutoff freq for bilinear Z transform
	float warp_cutoff = 2 * sample_rate * tan(radian_cutoff / 2 * sample_rate);
	// intermediate value
	float omega_T = warp_cutoff / sample_rate;
	// denominator
	iir->a1 = (2 - omega_T) / (2 + omega_T);
	// numerator values
	iir->b0 = omega_T / (2 + omega_T);
	iir->b1 = iir->b0;
	iir->x1 = 0;
	iir->y1 = 0;
}

float UpdateIIROrder1(IIR_Order1_struct *iir, float x0) {
    float result = 0;
	result += x0 * iir->b0;
	result += iir->x1 * iir->b1;
	result += iir->y1 * iir->a1;
	iir->x1 = x0;
	iir->y1 = result;
	return result;
}
