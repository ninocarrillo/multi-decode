#include <stdio.h>
#include <stdint.h>
#include "ax25.h"
#include "log.h"
#include "crc.h"

void InitAX25(AX25_Receiver_struct *rx) {
    rx->PacketCount = 0;
    rx->BitIndex = 0;
    rx->WordIndex = 0;
    rx->OneCounter = 0;
}

void AX25Receive(FILE *logfile, AX25_Receiver_struct *rx, int data) {
    int j;
    for (j = 0; j < 8; j++) { //step through each bit, MSB first
        // bit un-stuff
        if (data & 0x80) { // one bit
            rx->WorkingWord8 |= 0x80;
            rx->OneCounter++;
            rx->BitIndex++;
            if (rx->OneCounter > 6){
                // abort frame for invalid bit sequence
                //rx->OneCounter = 0; // Commenting this line out allows invalid 0xFE frame start flag.
                rx->BitIndex = 0;
                rx->WordIndex = 0;
            } else if (rx->BitIndex > 7) { // 8 valid bits received, record byte
                rx->BitIndex = 0;
                rx->Buffer[rx->WordIndex] = rx->WorkingWord8 & 0xFF;
                rx->WordIndex++;
                if (rx->WordIndex >= AX25_MAX_RX_BUFFER) {
                    // Frame exceeds size of buffer
                    rx->WordIndex = 0;
                    rx->OneCounter = 0;
                }                        
            } else {
                rx->WorkingWord8 >>= 1;
            }                    
        } else { //zero bit
            if (rx->OneCounter < 5) {
                rx->WorkingWord8 &= 0x7F;
                rx->BitIndex++;
                if (rx->BitIndex > 7) {
                    rx->BitIndex = 0;
                    rx->Buffer[rx->WordIndex] = rx->WorkingWord8 & 0xFF;
                    rx->WordIndex++;
                    if (rx->WordIndex >= AX25_MAX_RX_BUFFER) {
                        // Frame exceeds size of buffer
                        rx->WordIndex = 0;
                    }
                } else {
                    rx->WorkingWord8 >>= 1;
                }
            } else if (rx->OneCounter == 5) {
                // ignore stuffed zero
            } else if (rx->OneCounter == 6) { // Flag frame end
                if ((rx->WordIndex >= MIN_AX25_FRAME_LENGTH) && (rx->BitIndex == 7)) {
                    LogNewline(logfile);
                    LogString(logfile, "Candidate Packet Length: ");
                    LogInt(logfile, rx->WordIndex);
                    LogString(logfile, "\n");
                    for (int i = 0; i < rx->WordIndex; i++) {
                        LogHexByte(logfile, rx->Buffer[i]);
                    }
                    rx->WordIndex -= 2;
                    if (CCITT16CheckCRC(rx->Buffer, rx->WordIndex)) {
                        rx->PacketCount++;
                        rx->CRC = (uint16_t)rx->Buffer[rx->WordIndex];
                        rx->CRC |= ((uint16_t)rx->Buffer[rx->WordIndex + 1]) << 8;

                        LogNewline(logfile);
                        LogString(logfile, "Received Packet with CRC: ");
                        LogHexByte(logfile, rx->CRC >> 8);
                        LogHexByte(logfile, rx->CRC & 0xFF);
                        LogString(logfile, " Count: ");
                        LogInt(logfile, rx->PacketCount);
                        LogString(logfile, "\n");
                        for (int i = 0; i < rx->WordIndex; i++) {
                            LogHexByte(logfile, rx->Buffer[i]);
                        }

                    } 

                }
                rx->WordIndex = 0;
                rx->BitIndex = 0;
            } else { // one counter > 6, therefore frame is invalid
                rx->WordIndex = 0;
                rx->BitIndex = 0;
            }
            rx->OneCounter = 0;
        }
        data <<= 1;
    }

}
