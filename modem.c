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
	
	if (arg_count < 5) {
		printf("Not enough arguments.\nUsage: modem <wavfile_name> <CMA span> <CMA gain> <result file>\n");
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
	int16_t buffer[READ_SIZE];
	int16_t buffer2[READ_SIZE];
	int16_t buffer3[READ_SIZE * 2];
	
	int cma_span = atoi(arg_values[2]);
	float cma_mu = atof(arg_values[3]);
	
	AFSKDemod_struct AFSKDemodulator;
	InitAFSK( \
		logfile, \
		&AFSKDemodulator, \
		file_header.SampleRate, \
		/* low cut freq */ 1000, \
		/* high cut freq */ 2600, \
		/* tone 1 freq */ 1200, \
		/* tone 2 freq */ 2200, \
		/* symbol rate */ 1200, \
		/* output filter cutoff freq */ 1100, \
		/* equalizer span */ cma_span, \
		/* equalizer gain mu */ cma_mu \
	);

	Data_Slicer_struct Slicer;
	InitSlice2( \
		&Slicer, \
		file_header.SampleRate, \
		/* symbol rate */ 1200, \
		/* feedback parameter */ 0.75 \
	);
	
	LFSR_struct LFSR;
	InitLFSR(3, 0, &LFSR);

	AX25_Receiver_struct AX25_Receiver;
	InitAX25(&AX25_Receiver);

	FILE *output_file = fopen("./output.wav", "wb");
	// Make this a stereo wav file.
	file_header.ChannelCount = 2;
	file_header.BlockSize = 16;
	file_header.BytesPerBlock = 2 * 16 / 8;
	file_header.BytesPerSec = file_header.SampleRate * 2 * 16 / 8;
	file_header.DataSize = (file_header.DataSize + AFSKDemodulator.SampleDelay) * 2;
	fwrite(&file_header, 1, 44, output_file);
	
	fseek(wav_file, 44, SEEK_SET);
	count = fread(&buffer, 2, READ_SIZE, wav_file);
	
	int interleave_count;
	long int data;
	LogNewline(logfile);
	LogString(logfile, "Sliced Data: ");

	NCO_struct NCO;
	InitNCO(&NCO, 16, 31, 7, file_header.SampleRate);
	LogNewline(logfile);
	LogString(logfile, "NCO data:");
	LogNewline(logfile);
	{
		int i;
		for (i = 0; i < 256; i++) {
			LogInt(logfile, i);
			LogFloat(logfile, NCO.Wavetable[i]);
			LogString(logfile, ", ");

		}
	}

	PLL_struct PLL;
	InitPLL(&PLL, file_header.SampleRate, 1050, 500, 0.01, 0.0001, 100);
	

	int flushed = 0;

	while (count > 0) {
		for (int i = 0; i < count; i++) {
			buffer[i] = DemodAFSK(logfile, &AFSKDemodulator, (float)buffer[i] / (float)65536, Slicer.MatchDCD);
			//data = Slice2Eq(&Slicer, &AFSKDemodulator.EQ, buffer[i]);
			data = Slice2(&Slicer, buffer[i]);
			if (data > 0) {
				// Apply differential decoder
				data = Unscramble(&LFSR, data, Slicer.AccumulatorBitWidth, Slicer.AccumulatorBitWidth);
				AX25Receive(logfile, &AX25_Receiver, data, Slicer.AccumulatorBitWidth);
				if (AX25_Receiver.NewPacket) {
					AX25_Receiver.NewPacket = 0;
					LogNewline(logfile);
					LogString(logfile, "Equalizer Taps: ");
					for (int i = 0; i < AFSKDemodulator.EQ.Filter.TapCount; i++) {
						LogComplex(logfile, AFSKDemodulator.EQ.Filter.Taps[i]);
					}
					LogNewline(logfile);
					LogFloat(logfile, AFSKDemodulator.EQ.mu);
					//ResetCMATaps(&AFSKDemodulator.EQ);
					//Slicer.MatchDCD = 0;
				}
			}
			buffer2[i] = 1024 *  creal(AFSKDemodulator.Buffer3.Buffer[AFSKDemodulator.Buffer3.Index]);
			

			//PutCB(&AFSKDemodulator.Buffer2, buffer[i]);
			//buffer2[i] = FilterCB(&AFSKDemodulator.Buffer2, &AFSKDemodulator.HilbertFilter);
			//buffer[i] = FilterCB(&AFSKDemodulator.Buffer2, &AFSKDemodulator.DelayFilter);
			buffer[i] = 32767*GetNCOSampleFromFreq(&NCO, 1000);
		}

		// Interleave the data for Stereo wav file.
		interleave_count = InterleaveInt16(buffer3, buffer, buffer2, count);
		fwrite(&buffer3, 2, interleave_count, output_file);
		count = fread(&buffer, 2, AFSKDemodulator.SampleDelay, wav_file);
		if ((count < 1) && (flushed == 0)) {
			flushed = 1;
			count = READ_SIZE;
			for (int i = 0; i < count; i++) {
				buffer[i] = 0;
			}

		}
	}
	printf("Total Count: %d\n", AX25_Receiver.PacketCount);
	
	fclose(output_file);
	fclose(wav_file);
	fclose(logfile);
	
	FILE *output_data_file = fopen(arg_values[4], "a");
	fprintf(output_data_file, "%s, %i, %f, %i\n", arg_values[1], cma_span, cma_mu, AX25_Receiver.PacketCount);
	fclose(output_data_file);

	return 0;
}
