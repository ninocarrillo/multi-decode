#include <stdint.h>
#include <limits.h>
#include "ted.h"
#include "dsp.h"

void InitSlice2(Data_Slicer_struct *slicer, float sample_rate, float symbol_rate, float lock_rate) {
    slicer->DCDLoad = 32;
    slicer->ClockStep = symbol_rate / sample_rate;
    slicer->Clock = 0;
    slicer->LockRate = lock_rate;
    slicer->BitIndex = 0;
    slicer->SyncDCD = 0;
    slicer->MatchDCD = 0;
	slicer->AccumulatorBitWidth = (sizeof(long int) * CHAR_BIT) - 1;
	slicer->DataAccumulator = 0;
}
int ZDetect(float old_y, float new_y) {
    int crossed = 0;
	float zero = 0;
    // Detect zero crossing.
    if (old_y > zero) {
        // Last sample was positive.
        if (new_y <= zero) {
            crossed = 1;
        }
    } else if (old_y <= zero) {
        // Last sample was negative.
        if (new_y > zero) {
            crossed = 1;
        }
    }
    return crossed;
}

long int Slice2Eq(Data_Slicer_struct *slicer, CMA_Equalizer_struct *eq, float sample) {
    long int result = -1;
    slicer->Clock += slicer->ClockStep;
    if (slicer->Clock > 0.5) {
        slicer->Clock -= 1;
        slicer->DataAccumulator <<= 1;
        slicer->BitIndex++;
        if (sample > 0) {
            slicer->DataAccumulator |= 1;
        }
        if (slicer->BitIndex >= slicer->AccumulatorBitWidth) {
            slicer->BitIndex = 0;
            result = slicer->DataAccumulator;
            slicer->DataAccumulator = 0;
        }
        slicer->MatchDCD--;
        if (slicer->MatchDCD < 0) {
            slicer->MatchDCD = -1;
        } 

        if ((slicer->DataAccumulator & 0xFFFFFF) == 0x808080) {
            slicer->MatchDCD = slicer->DCDLoad;
        }
        if ((slicer->DataAccumulator & 0xFFFFFF) == 0x7F7F7F) {
            slicer->MatchDCD = slicer->DCDLoad;
        }
        if ((slicer->DataAccumulator & 0xFFFFFF) == 0x555555) {
            slicer->MatchDCD = slicer->DCDLoad;
        }

        if (slicer->MatchDCD > 0) {
            // Apply equalizer feedback
            CMAFeedback(eq);
        }
    }
    if (ZDetect(slicer->LastSample, sample)) {
        slicer->Clock *= slicer->LockRate;
    }
    slicer->LastSample = sample;
    return result;
}

long int Slice2(Data_Slicer_struct *slicer, float sample) {
    long int result = -1;
    slicer->Clock += slicer->ClockStep;
    if (slicer->Clock > 0.5) {
        slicer->Clock -= 1;
        slicer->DataAccumulator <<= 1;
        slicer->BitIndex++;
        if (sample > 0) {
            slicer->DataAccumulator |= 1;
        }
        if (slicer->BitIndex >= slicer->AccumulatorBitWidth) {
            slicer->BitIndex = 0;
            result = slicer->DataAccumulator;
			slicer->DataAccumulator = 0;
        }
        slicer->MatchDCD--;
        if (slicer->MatchDCD < 0) {
            slicer->MatchDCD = -1;
        }
        if ((slicer->DataAccumulator & 0xFFFFFF) == 0x808080) {
            slicer->MatchDCD = slicer->DCDLoad;
        }
        if ((slicer->DataAccumulator & 0xFFFFFF) == 0x7F7F7F) {
            slicer->MatchDCD = slicer->DCDLoad;
        }
		if ((slicer->DataAccumulator & 0xFFFFFF) == 0x555555) {
            slicer->MatchDCD = slicer->DCDLoad;
        }
    }
	if (ZDetect(slicer->LastSample, sample)) {
		slicer->Clock *= slicer->LockRate;
	}
	slicer->LastSample = sample;
    return result;
}

