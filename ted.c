#include <stdint.h>
#include <limits.h>
#include "ted.h"
#include "dsp.h"

void InitSlice2(Data_Slicer_struct *slicer, float sample_rate, float symbol_rate, float lock_rate) {
    slicer->DCDLoad = sample_rate / 20;
    slicer->ClockStep = symbol_rate / sample_rate;
    slicer->Clock = 0;
    slicer->LockRate = lock_rate;
    slicer->BitIndex = 0;
    slicer->SyncDCD = 0;
    slicer->MatchDCD = 0;
	slicer->AccumulatorBitWidth = (sizeof(long int) * CHAR_BIT) - 1;
	slicer->DataAccumulator = 0;
}

void InitGardnerLinear(Gardner_TED_struct *ted, int sample_rate, int symbol_rate, float loop_filter_cutoff) {
	InitIIROrder1(&ted->LoopFilter, sample_rate, loop_filter_cutoff);
	ted->AccumulatorBitWidth = (sizeof(long int) * CHAR_BIT) - 1;
    ted->DataAccumulator = 0;
	ted->Oversample = sample_rate / symbol_rate;
	ted->SampleFractionalTarget = 0.5; /* constrain 0.0 to 1.0 */
	ted->SampleBaseTarget = ted->Oversample - 1; /* constrain 0 to oversample - 1 */
	ted->ZeroBaseTarget = ted->SampleBaseTarget - (ted->Oversample / 2); /* constrain 0 to oversample - 1 */
	ted->SampleIndex = 0; /* free-running counter incremented once per sample */
	/* Constrain SampleIndex from 0 to oversample - 1 */
	ted->BitIndex = 0;
	ted->DCDLoad = 32;
	ted->MatchDCD = 0;

}

void InitSliceN(Data_Slicer_struct *slicer, float sample_rate, float symbol_rate, float lock_rate, int bits_per_sym) {
    slicer->DCDLoad = 32;
    slicer->ClockStep = symbol_rate / sample_rate;
    slicer->Clock = 0;
    slicer->LockRate = lock_rate;
    slicer->BitIndex = 0;
    slicer->SyncDCD = 0;
    slicer->MatchDCD = 0;
	slicer->AccumulatorBitWidth = (sizeof(long int) * CHAR_BIT) - 1;
	slicer->DataAccumulator = 0;
	slicer->BitsPerSymbol = bits_per_sym;
	slicer->Interval = (float)1/(float)(1<<bits_per_sym);
	slicer->SymbolTap = 1 << (bits_per_sym - 1);
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


long int GardnerLinear(Gardner_TED_struct *ted, float S0) {
    long int result = -1;

	/* Interpolate */
	float I0 = (ted->SampleFractionalTarget * S0) + ((1-ted->SampleFractionalTarget) * ted->S1);
	ted->S1 = S0;
	
	ted->SampleIndex++;
	if (ted->SampleIndex >= ted->Oversample) {
		ted->SampleIndex = 0;
	}
	if (ted->SampleIndex == ted->SampleBaseTarget) {
		/* Save data */
        ted->DataAccumulator <<= 1;
        ted->BitIndex++;
        if (I0 > 0) {
            ted->DataAccumulator |= 1;
        }
        if (ted->BitIndex >= ted->AccumulatorBitWidth) {
            ted->BitIndex = 0;
            result = ted->DataAccumulator;
			ted->DataAccumulator = 0;
        }
		
        ted->MatchDCD--;
        if (ted->MatchDCD < 0) {
            ted->MatchDCD = -1;
        }
        if ((ted->DataAccumulator & 0xFFFFFF) == 0x808080) {
            ted->MatchDCD = ted->DCDLoad;
        }
        if ((ted->DataAccumulator & 0xFFFFFF) == 0x7F7F7F) {
            ted->MatchDCD = ted->DCDLoad;
        }
		if ((ted->DataAccumulator & 0xFFFFFF) == 0x555555) {
            ted->MatchDCD = ted->DCDLoad;
        }		
		
		
		
	}

	if (ted->SampleIndex == ted->ZeroBaseTarget) {
		/* Apply Gerdner's equation */
		float error = ted->I1 * (I0 - ted->I2);
		/* Filter error value */
		float feedback = UpdateIIROrder1(&ted->LoopFilter, error);
		/* Apply feedback to SampleTarget */
		ted->SampleFractionalTarget -= (feedback * 0.17/32768);
		while (ted->SampleFractionalTarget < 0.0) {
			ted->SampleBaseTarget -= 1;
			ted->SampleFractionalTarget += 1.0;
		}
		while (ted->SampleFractionalTarget > 1.0) {
			ted->SampleBaseTarget += 1;
			ted->SampleFractionalTarget -= 1.0;
		}
		while (ted->SampleBaseTarget < 0) {
			ted->SampleBaseTarget += ted->Oversample;
		}
		while (ted->SampleBaseTarget >= ted->Oversample) {
			ted->SampleBaseTarget -= ted->Oversample;
		}
		ted->ZeroBaseTarget = ted->SampleBaseTarget - (ted->Oversample / 2);
		while (ted->ZeroBaseTarget < 0) {
			ted->ZeroBaseTarget += ted->Oversample;
		}
	}
	/* update interpolated sample history */
	ted->I2 = ted->I1;
	ted->I1 = I0;
    return result;
}


long int SliceN(Data_Slicer_struct *slicer, float sample) {
    long int result = -1;
	
	// This function is called onced per sample.
	// Returns -1 if there is not yet a full accumulator of data (machine width).
	// Returns a full accumulator of data once available.
	// Some latent data is stored in the structure between calls.
	// Requires that machine accumulator width is greater than symbol width (bits).
	
	while (slicer->BitsLeftInSymbol > 0) {
		slicer->DataAccumulator <<= 1;
		if (slicer->SymbolTap & slicer->Symbol) {
			slicer->DataAccumulator |= 1;
		}
		slicer->Symbol <<= 1;
		slicer->BitsLeftInSymbol--;
		slicer->BitIndex++;
        if (slicer->BitIndex >= slicer->AccumulatorBitWidth) {
            slicer->BitIndex = 0;
            result = slicer->DataAccumulator;
			slicer->DataAccumulator = 0;
        }
	}
	
    slicer->Clock += slicer->ClockStep;
	
    if (slicer->Clock > 0.5) {
        slicer->Clock -= 1;
		
		// Sample and save this symbol.
		//sample = -sample;
		if (sample < -400) {
			// symbol -3 demaps to 11
			slicer->Symbol = 3;
		} else if (sample < 0) {
			// symbol -1 demaps to 10
			slicer->Symbol = 2;
		} else if (sample < 400) {
			// symbol +1 demaps to 00
			slicer->Symbol = 0;
		} else {
			slicer->Symbol = 1;
		}
		slicer->BitsLeftInSymbol = slicer->BitsPerSymbol;
		
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


