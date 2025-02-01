#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <complex.h>
#include <math.h>
#include <time.h>
#include "log.h"
#include "modem.h"
#include "dsp.h"

int main(int arg_count, char* arg_values[]) {
	FILE *logfile;
	logfile = fopen("md.log", "w");
	LogNewline(logfile);
	LogString(logfile, "Opened logfile.");

	if (!logfile) {
		printf("Unable to create logfile, exiting.\n");
	}
	LogNewline(logfile);
	LogString(logfile, "Invoke arguments: ");
	for (int i = 0; i < arg_count; i++) {
		LogString(logfile, arg_values[i]);
	}
	
	if (arg_count < 2) {
		printf("Not enough arguments.\nUsage: modem <wavfile_name>\n");
		LogString(logfile, "Not enough arguments, exiting.\n");
		return -1;
	}

	WAVHeader_struct file_header;
	FILE *wav_file = fopen(arg_values[1], "rb");
	if (!wav_file) {
		printf("Unable to open file %s, exiting.\n", arg_values[1]);
		LogNewline(logfile);
		LogString(logfile, "Unable to open wavefile: ");
		LogString(logfile, arg_values[1]);
		return -1;
	}
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
	
	AFSKDemod_struct AFSKDemodulator;
	InitAFSK( \
		logfile, \
		&AFSKDemodulator, \
		file_header.SampleRate, \
		/* low cut freq */ 900, \
		/* high cut freq */ 2500, \
		/* tone 1 freq */ 1200, \
		/* tone 2 freq */ 2200, \
		/* symbol rate */ 1200 \
	);
	
	
	// Initialize the hilbert filter
	FIR_struct HilbertFilter;
	FIR_struct DelayFilter;
	int hilbert_delay = InitHilbert(&HilbertFilter, &DelayFilter, 11);

	FILE *output_file = fopen("./output.wav", "wb");
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
		for (int i = 0; i < count; i++) {
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
	fclose(logfile);

	return 0;
}
