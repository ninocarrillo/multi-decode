#ifndef iir_h
#define iir_h

typedef struct {
    float a1;
    float b0;
    float b1;
    float x1;
    float y1;
} IIR_Order1_struct;

float UpdateIIROrder1(IIR_Order1_struct *, float);
#endif