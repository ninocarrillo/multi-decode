#include "lfsr.h"
#include <stdio.h>

// Unscrambles one word, MSB first. Uses Galois configuration.
long int Unscramble(LFSR_struct *LFSR, long int data, int in_bit_count, int target_width) {
	long int target_mask = 1;
	target_mask = (target_mask << target_width) - 1;
	long int input_mask = 1;
	input_mask = input_mask << (target_width - 1);
	int j = 0;
	for (int i = 0; i < in_bit_count; i++) {
		long int input = data & input_mask;
        data <<= 1;
        if (input) {
            LFSR->ShiftRegister ^= LFSR->Polynomial;
        }
		long int output;
        if (LFSR->Invert) {
            output = (~LFSR->ShiftRegister) & LFSR->FeedbackMask;
        } else {
            output = LFSR->ShiftRegister & LFSR->FeedbackMask;
        }

        if (output) {
            data |= 1;
        }
        j++;
        if (j == target_width) {
            data &= target_mask;
        }
        LFSR->ShiftRegister >>= 1;
    }
	return data;
}

void InitLFSR(long int Polynomial, int invert, LFSR_struct *LFSR) {
    LFSR->Polynomial = Polynomial;
    LFSR->TapCount = 0;
    // Index lowest power taps in lowest order positions.
    for (int i = 0; i < MAXPOWER; i++) {
        if ((LFSR->Polynomial >> i) & 1) {
            LFSR->Tap[LFSR->TapCount++] = i;
        }
    }
    LFSR->InputMask = (long int)1 << (LFSR->Tap[LFSR->TapCount - 1]);
    LFSR->OutputMask = (long int)1 << LFSR->Tap[1];
    LFSR->FeedbackMask = 1;
    LFSR->BitDelay = LFSR->Tap[LFSR->TapCount - 1] - LFSR->Tap[1];
    LFSR->BitsInProgress = 0;
    LFSR->Initialized = 1;
    LFSR->Order = 1 << LFSR->Tap[LFSR->TapCount - 1];
    LFSR->ShiftRegister = -1;
	LFSR->Invert = invert;
}