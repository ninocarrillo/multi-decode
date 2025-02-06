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
		/* low cut freq */ 1000, \
		/* high cut freq */ 2800, \
		/* tone 1 freq */ 1200, \
		/* tone 2 freq */ 2200, \
		/* symbol rate */ 1200, \
		/* output filter cutoff freq */ 1000, \
		/* equalizer gain mu */ /*0.002*/ 0.00 \
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
	fwrite(&file_header, 1, 44, output_file);
	
	fseek(wav_file, 44, SEEK_SET);
	count = fread(&buffer, 2, READ_SIZE, wav_file);
	
	int interleave_count;
	long int data;
	LogNewline(logfile);
	LogString(logfile, "Sliced Data: ");

	while (count > 0) {
		for (int i = 0; i < count; i++) {
			buffer[i] = DemodAFSK(logfile, &AFSKDemodulator, buffer[i], Slicer.MatchDCD);
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
				}
			}
			buffer2[i] = Slicer.MatchDCD;

			//PutCB(&AFSKDemodulator.Buffer2, buffer[i]);
			//buffer2[i] = FilterCB(&AFSKDemodulator.Buffer2, &AFSKDemodulator.HilbertFilter);
			//buffer[i] = FilterCB(&AFSKDemodulator.Buffer2, &AFSKDemodulator.DelayFilter);
		}

		// Interleave the data for Stereo wav file.
		interleave_count = InterleaveInt16(buffer3, buffer, buffer2, count);
		fwrite(&buffer3, 2, interleave_count, output_file);
		count = fread(&buffer, 2, READ_SIZE, wav_file);
	}
	printf("Total Count: %d", AX25_Receiver.PacketCount);
	
	fclose(output_file);
	fclose(wav_file);
	fclose(logfile);

	return 0;
}
