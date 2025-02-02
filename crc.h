#define CCITT16_CRC_POLY 0x8408 // bit-reversed 0x11021, MSB testing
#define CCITT32_CRC_POLY 0xEDB88320 // bit-reversed 0x104C11DB6, MSB testing

uint16_t CCITT16CalcCRC(uint8_t* data, int);
int CCITT16CheckCRC(uint8_t* data, int);
