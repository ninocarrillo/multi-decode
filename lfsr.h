#ifndef lfsr_h
#define lfsr_h
#include <stdint.h>


#define MAXTAPS 64
#define MAXPOWER 64


typedef struct {
    long int ShiftRegister;
    long int Polynomial;
    long int FeedbackMask;
    long int InputMask;
    long int OutputMask;
	long int Tap[MAXTAPS];    
	int Order;
    int TapCount;
    int BitDelay;
    int BitsInProgress;
    int Initialized;
    int Invert;
} LFSR_struct;

long int Unscramble(LFSR_struct *, long int, int, int);
void InitLFSR(long int, int, LFSR_struct *);
unsigned long int GetPRN(LFSR_struct *);
#endif