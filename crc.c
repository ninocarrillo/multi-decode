#include <stdint.h>
#include "crc.h"

uint16_t CCITT16CalcCRC(uint8_t* data, int count) {
    int i, j;
    uint16_t x;
    uint16_t fcsval = 0xFFFF;
    unsigned fcsbit;
    for (i = 0; i < count; i++) {
        x = data[i];
        for (j = 0; j < 8; j++) {
            fcsbit = 0;
            if (fcsval & 1) {
                fcsbit = 1;
            }
            fcsval >>= 1;            
            if (fcsbit ^ (x & 1)) {
                fcsval ^= CCITT16_CRC_POLY;
            }
            x >>= 1;
        }
    }
    return ~fcsval;
}

int CCITT16CheckCRC(uint8_t* data, int count) {
    int result;
    uint16_t x, y;
    uint16_t FCS = CCITT16CalcCRC(data, count);
    x = data[count];
    y = data[count + 1] & 0xFF;
    x |= y << 8;
    if (FCS == x) {
        result = 1;
    } else {
        result = 0;
    }
    return result;
}
