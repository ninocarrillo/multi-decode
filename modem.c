#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <complex.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#include "log.h"
#include "modem.h"
#include "dsp.h"
#include "ted.h"
#include "ax25.h"
#include "lfsr.h"
#include "nco.h"
#include "pll.h"
#include "afsk.h"

#define WRITE_WAV_OUT 0

int main(int arg_count, char* arg_values[]) {
	
	clock_t start_time, end_time;
	 double cpu_time_used;
	start_time = clock(); // Record the start time
	
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
	
	if (arg_count < 6) {
		printf("Not enough arguments.\nUsage: modem <wavfile_name> <CMA span> <CMA gain> <decoder type> <result file>\n");
		printf("decoder type 1: AFSK 1200 Correlator\n");
		printf("decoder type 2: AFSK 1200 PLL\n");
		printf("decoder type 2: AFSK 1200 Quadrature\n");
		LogString(logfile, "Not enough arguments, exiting.\n");
		return -1;
	}
	
	printf("\n");
	for(int i = 0; i < arg_count; i++) {
		printf("%s ", arg_values[i]);
	}
	printf("\n");

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
#define READ_SIZE 65536
	int16_t buffer1[READ_SIZE];
	int16_t buffer2[READ_SIZE];
	int16_t buffer3[READ_SIZE * 2];
	
	int cma_span = atoi(arg_values[2]);
	float cma_mu = atof(arg_values[3]);
	//float cma_mu = 0;
	float pll_p_gain = atof(arg_values[3]);
	int decoder_type = atoi(arg_values[4]);
	
	float low_cut = 800;
	float high_cut = 2500;

	AFSKQuadDemod_struct AFSKQuadDemodulator1;
	InitAFSKQuad( \
		logfile, \
		&AFSKQuadDemodulator1, \
		file_header.SampleRate, \
		/* low cut freq */ low_cut, \
		/* high cut freq */ high_cut, \
		/* output filter cutoff freq */ 1100, \
		/* equalizer span */ 1, \
		/* equalizer gain mu */ 0 \
	);
	AFSKQuadDemod_struct AFSKQuadDemodulator2;
	InitAFSKQuad( \
		logfile, \
		&AFSKQuadDemodulator2, \
		file_header.SampleRate, \
		/* low cut freq */ low_cut, \
		/* high cut freq */ high_cut, \
		/* output filter cutoff freq */ 1100, \
		/* equalizer span */ cma_span, \
		/* equalizer gain mu */ cma_mu \
	);

	AFSKPLLDemod_struct AFSKPLLDemodulator1;
	InitAFSKPLL( \
		logfile, \
		&AFSKPLLDemodulator1, \
		file_header.SampleRate, \
		/* low cut freq */ low_cut, \
		/* high cut freq */ high_cut, \
		/* pll set freq */ 1700, \
		/* pll loop cutoff */ /*3800*/3000, \
		/* pll_p_gain */9500, \
		/* pll_i_gain */ 0, \
		/* pll_i_limit */ 50, \
		/* output filter cutoff freq */ 1100, \
		/* equalizer span */ 1, \
		/* equalizer gain mu */ 0 \
	);	
	AFSKPLLDemod_struct AFSKPLLDemodulator2;
	InitAFSKPLL( \
		logfile, \
		&AFSKPLLDemodulator2, \
		file_header.SampleRate, \
		/* low cut freq */ low_cut, \
		/* high cut freq */ high_cut, \
		/* pll set freq */ 1700, \
		/* pll loop cutoff */ 3000, \
		/* pll_p_gain */9500, \
		/* pll_i_gain */ 0, \
		/* pll_i_limit */ 50, \
		/* output filter cutoff freq */ 1100, \
		/* equalizer span */ cma_span, \
		/* equalizer gain mu */ cma_mu \
	);	
	
	float center_tone = 1700;
	float tone_1 = center_tone - 250;
	float tone_2 = center_tone + 250;

	AFSKDemod_struct AFSKDemodulator1;
	InitAFSK( \
		logfile, \
		&AFSKDemodulator1, \
		file_header.SampleRate, \
		/* low cut freq */ low_cut, \
		/* high cut freq */ high_cut, \
		/* tone 1 freq */ tone_1, \
		/* tone 2 freq */ tone_2, \
		/* symbol rate */ 1200, \
		/* output filter cutoff freq */ 1100, \
		/* equalizer span */ 1, \
		/* equalizer gain mu */ 0 \
	);
	AFSKDemod_struct AFSKDemodulator2;
	InitAFSK( \
		logfile, \
		&AFSKDemodulator2, \
		file_header.SampleRate, \
		/* low cut freq */ low_cut, \
		/* high cut freq */ high_cut, \
		/* tone 1 freq */ tone_1, \
		/* tone 2 freq */ tone_2, \
		/* symbol rate */ 1200, \
		/* output filter cutoff freq */ 1100, \
		/* equalizer span */ cma_span, \
		/* equalizer gain mu */ cma_mu \
	);

	Data_Slicer_struct Slicer1;
	InitSlice2( \
		&Slicer1, \
		file_header.SampleRate, \
		/* symbol rate */ 1200, \
		/* feedback parameter */ 0.75 \
	);
	Data_Slicer_struct Slicer2;
	InitSlice2( \
		&Slicer2, \
		file_header.SampleRate, \
		/* symbol rate */ 1200, \
		/* feedback parameter */ 0.75 \
	);

	Gardner_TED_struct Gardner1;
	Gardner_TED_struct Gardner2;
	InitGardnerLinear( \
		&Gardner1, \
		file_header.SampleRate,
		1200 \
	); 
	InitGardnerLinear( \
		&Gardner2, \
		file_header.SampleRate, \
		1200 \
	); 
	
	Data_Slicer_struct NSlicer1;
	InitSliceN( \
		&NSlicer1, \
		file_header.SampleRate, \
		/* symbol rate */ 1200, \
		/* feedback parameter */ 0.9, \
		/* bits per symbol */ 2 \
	);
	Data_Slicer_struct NSlicer2;
	InitSliceN( \
		&NSlicer2, \
		file_header.SampleRate, \
		/* symbol rate */ 1200, \
		/* feedback parameter */ 0.9, \
		/* bits per symbol */ 2 \
	);
	
	// G3RUH Pily is 0x21001 with Inversion
	// Differential poly is 0x3
	// Normal AFSK does not have Inversion
	LFSR_struct LFSR1;
	InitLFSR(0x3, 0, &LFSR1);
	LFSR_struct LFSR2;
	InitLFSR(0x3, 0, &LFSR2);

	AX25_Receiver_struct AX25_Receiver1;
	InitAX25(&AX25_Receiver1);
	AX25_Receiver1.ID = 1;
	AX25_Receiver_struct AX25_Receiver2;
	InitAX25(&AX25_Receiver2);
	AX25_Receiver2.ID = 2;

	FILE *output_file = NULL;
	if (WRITE_WAV_OUT) {
		output_file = fopen("./output.wav", "wb");
		// Make this a stereo wav file.
		file_header.ChannelCount = 2;
		file_header.BlockSize = 16;
		file_header.BytesPerBlock = 2 * 16 / 8;
		file_header.BytesPerSec = file_header.SampleRate * 2 * 16 / 8;
		file_header.DataSize = (file_header.DataSize + AFSKDemodulator1.SampleDelay) * 2;
		fwrite(&file_header, 1, 44, output_file);
	}
	fseek(wav_file, 44, SEEK_SET);
	count = fread(&buffer1, 2, READ_SIZE, wav_file);

	int flushed = 0;

	int interleave_count;
	long int data;
	LogNewline(logfile);
	LogString(logfile, "Sliced Data: ");
	float buffer_sum = 0;
	float buffer_count = 0;
	while (count > 0) {
		for (int i = 0; i < count; i++) {
			float input_sample = (float)buffer1[i];
			if (decoder_type == 1) {
				buffer1[i] = DemodAFSK(logfile, &AFSKDemodulator1, input_sample / (float)65536, Slicer1.MatchDCD);
			} else if (decoder_type == 2) {
				buffer1[i] = DemodAFSKPLL(logfile, &AFSKPLLDemodulator1, input_sample / (float)65536, Slicer1.MatchDCD);
			} else if (decoder_type == 3) {
				buffer1[i] = DemodAFSKQuad(logfile, &AFSKQuadDemodulator1, input_sample / (float)65536, Slicer1.MatchDCD);
			}
			//data = Slice2(&Slicer1, buffer1[i]);
			data = GardnerLinear(&Gardner1, buffer1[i]);
			if (data > 0) {
				data = Unscramble(&LFSR1, data, Slicer1.AccumulatorBitWidth, Slicer1.AccumulatorBitWidth);
				AX25Receive(logfile, &AX25_Receiver1, data, Slicer1.AccumulatorBitWidth, AX25_Receiver2.CRC);
				if (AX25_Receiver1.CRC == AX25_Receiver2.CRC) {
					AX25_Receiver2.CRC = -1;
					AX25_Receiver1.CRC = -1;
				}
			}


			if (decoder_type == 1) {
				buffer2[i] = DemodAFSK(logfile, &AFSKDemodulator2, input_sample / (float)65536, Slicer2.MatchDCD);
			} else if (decoder_type == 2) {
				buffer2[i] = DemodAFSKPLL(logfile, &AFSKPLLDemodulator2, input_sample / (float)65536, Slicer2.MatchDCD);
			} else if (decoder_type == 3) {
				buffer2[i] = DemodAFSKQuad(logfile, &AFSKQuadDemodulator2, input_sample / (float)65536, Slicer2.MatchDCD);
			}
			data = Slice2(&Slicer2, buffer2[i]);
			if (data > 0) {
				data = Unscramble(&LFSR2, data, Slicer2.AccumulatorBitWidth, Slicer2.AccumulatorBitWidth);
				AX25Receive(logfile, &AX25_Receiver2, data, Slicer2.AccumulatorBitWidth, AX25_Receiver1.CRC);
				if (AX25_Receiver1.CRC == AX25_Receiver2.CRC) {
					AX25_Receiver1.CRC = -1;
					AX25_Receiver2.CRC = -1;
				}
			}




		}
		
		
		if (WRITE_WAV_OUT) {
			// Interleave the data for Stereo wav file.
			interleave_count = InterleaveInt16(buffer3, buffer1, buffer2, count);
			fwrite(&buffer3, 2, interleave_count, output_file);
		}
		
		count = fread(&buffer1, 2, AFSKDemodulator1.SampleDelay, wav_file);
		if ((count < 1) && (flushed == 0)) {
			flushed = 1;
			count = READ_SIZE;
			for (int i = 0; i < count; i++) {
				buffer1[i] = 0;
			}

		}
	}

	printf("Receiver 1 count: %d\n", AX25_Receiver1.PacketCount);
	printf("Receiver 2 count: %d\n", AX25_Receiver2.PacketCount);
	printf("Unique count: %d\n", AX25_Receiver1.UniquePacketCount + AX25_Receiver2.UniquePacketCount);
	
	fclose(output_file);
	if (WRITE_WAV_OUT) {
		fclose(wav_file);
	}
	fclose(logfile);
	
	FILE *output_data_file = fopen(arg_values[5], "a");
	fprintf(output_data_file, "%s, %i, %f, %i, %i, %i, %i, %i\n", arg_values[1], cma_span, cma_mu, AX25_Receiver1.PacketCount, AX25_Receiver1.UniquePacketCount, AX25_Receiver2.PacketCount, AX25_Receiver2.UniquePacketCount, AX25_Receiver1.UniquePacketCount + AX25_Receiver2.UniquePacketCount);
	fclose(output_data_file);

	end_time = clock(); // Record the end time

	cpu_time_used = ((double) (end_time - start_time)) / CLOCKS_PER_SEC;
	printf("Elapsed CPU time: %f seconds\n", cpu_time_used);

	return 0;
}
