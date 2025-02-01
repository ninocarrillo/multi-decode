typedef struct {
	char FileTypeBlockID[4];
	uint32_t FileSize;
	char FileFormatID[4];
	char FormatBlockID[4];
	uint32_t BlockSize;
	uint16_t AudioFormat;
	uint16_t ChannelCount;
	uint32_t SampleRate;
	uint32_t BytesPerSec;
	uint16_t BytesPerBlock;
	uint16_t BitsPerSample;
	char DataBlockID[4];
	uint32_t DataSize;
} WAVHeader_struct;

