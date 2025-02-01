#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <complex.h>
#include <math.h>
#include "modem.h"
#include "dsp.h"

int main(void) {
	WAVHeader_struct file_header;
	printf("Size of WAV Header struct: %lu Bytes\n", sizeof(file_header));
	
	FILE *wav_file = fopen("./tone.wav", "rb");
	fread(&file_header, 1, 44, wav_file);
	
	printf("Sample Rate: %d\n", file_header.SampleRate);
	printf("Channel Count: %d\n", file_header.ChannelCount);
	printf("Audio Format: %04X\n", file_header.AudioFormat);
	printf("Bits Per Sample: %d\n", file_header.BitsPerSample);
	printf("Block Size: %d\n", file_header.BlockSize);
	printf("Bytes Per Block: %d\n", file_header.BytesPerBlock);
	printf("Data Size: %d\n", file_header.DataSize);
	int32_t count;
#define READ_SIZE 4096
	int16_t buffer[READ_SIZE];
	int16_t buffer2[READ_SIZE];
	int16_t buffer3[READ_SIZE * 2];
	count = fread(&buffer, 1, READ_SIZE, wav_file);
	int32_t total = 0;
	while (count > 0) {
		total += count;
		//printf("Read %d bytes\n", count);
		count = fread(&buffer, 1, READ_SIZE, wav_file);
	}
	printf("Size of final read: %d\n", count);
	printf("Total bytes read: %d\n", total);


	printf("Reading some data.\n");
	fseek(wav_file, 44, SEEK_SET);
	count = fread(&buffer, 2, 1000, wav_file);
	int i = 0;
	for (i = 0; i < count; i++) {
		printf("%d\n", buffer[i]);
	}
	
	// Initialize the hilbert filter
	FIR_struct HilbertFilter;
	FIR_struct DelayFilter;
	int hilbert_delay = InitHilbert(&HilbertFilter, &DelayFilter, 11);
	printf("Hilbert Filter Tap Count: %d\n", HilbertFilter.TapCount);
	for (i = 0; i < HilbertFilter.TapCount; i++) {
		printf("%f,", HilbertFilter.Taps[i]);
	}

	printf("\nDelay Filter Tap Count: %d\n", DelayFilter.TapCount);
	for (i = 0; i < DelayFilter.TapCount; i++) {
		printf("%f,", DelayFilter.Taps[i]);
	}
	
	FILE *output_file = fopen("./t1x.wav", "ab+");
	// Make this a stereo wav file.
	file_header.ChannelCount = 2;
	file_header.BlockSize = 16;
	file_header.BytesPerBlock = 2 * 16 / 8;
	file_header.BytesPerSec = file_header.SampleRate * 2 * 16 / 8;
	fwrite(&file_header, 1, 44, output_file);
	
	fseek(wav_file, 44, SEEK_SET);
	count = fread(&buffer, 2, READ_SIZE, wav_file);
	CircularBuffer_struct CB1, CB2, CB3;
	CB1.Length = MAX_FIR_TAP_COUNT;
	CB1.Index = 0;
	CB2.Length = MAX_FIR_TAP_COUNT;
	CB2.Index = 0;
	CB3.Length = MAX_FIR_TAP_COUNT;
	CB3.Index = 0;
	FIR_struct Filter;
	GenBandFIR(&Filter, 300, 3000, file_header.SampleRate, 101);
	
	int interleave_count;
	while (count > 0) {
		for (i = 0; i < count; i++) {
			// Put data in circ buff
			PutCB(&CB1, (float)buffer[i]);
			// Band-pass filter the data and put it in the next CB.
			PutCB(&CB2, FilterCB(&CB1, &Filter));
			// Apply Hilbert transform.
			buffer[i] = (int16_t)FilterCB(&CB2, &HilbertFilter);
			// Delay the real data.
			buffer2[i] = (int16_t)FilterCB(&CB2, &DelayFilter);
			//printf("%.3f\n", sqrt(pow(buffer[i], 2) + pow(buffer2[i], 2)));
		}
		// Interleave the data for Stereo wav file.
		interleave_count = InterleaveInt16(buffer3, buffer, buffer2, count);
		fwrite(&buffer3, 2, interleave_count, output_file);
		count = fread(&buffer, 2, READ_SIZE, wav_file);
	}
	
	fclose(output_file);
	fclose(wav_file);

	printf("\nMade a FIR.\n");
	printf("Sample Rate: %.2f\n", Filter.SampleRate);
	printf("Tap Count: %d\n", Filter.TapCount);
	printf("Gain: %6f\n", Filter.Gain);
	printf("Taps: ");
	for (i = 0; i < Filter.TapCount; i++) {
		printf(" %.8f, ", Filter.Taps[i]);
	}

	printf("\nComplex Numbers:\n");
	float complex x = 1 + 2*I;
	printf("Complex number x = %.2f + %.2fi\n", creal(x), cimag(x));
	x = x * 7;
	printf("Scaling x by factor of 7: x = %.2f + %.2fi\n", creal(x), cimag(x));

	return 0;
}
