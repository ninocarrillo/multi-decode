#include "iir.h"

void InitIIROrder1(IIR_Order1_struct *iir, float cutoff_freq) {

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
