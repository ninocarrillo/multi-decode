#define AX25_MAX_RX_BUFFER 2048
#define MIN_AX25_FRAME_LENGTH 16

typedef struct {
    int BitIndex;
    int WordIndex;
    int OneCounter;
    int CRC;
    int WorkingWord8;
    int RxByteCount;
    uint8_t Buffer[AX25_MAX_RX_BUFFER];
    int PacketCount;
} AX25_Receiver_struct;

void InitAX25(AX25_Receiver_struct *);
void AX25Receive(FILE *, AX25_Receiver_struct *, int);