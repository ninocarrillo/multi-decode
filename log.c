#include <stdio.h>
#include <time.h>

#include "log.h"

void LogNewline(FILE *file) {
	char time_string[80];
	struct tm* time_now;
	time_t t = time(NULL);
	time_now = localtime(&t);
	strftime(time_string, sizeof(time_string), "%c", time_now);
	fprintf(file, "\n%s --- ", time_string);
}

void LogString(FILE *file, char* message) {
	fprintf(file, "%s", message);
}

void LogFloat(FILE *file, float value) {
	fprintf(file, " %f", value);
}

void LogInt(FILE *file, int value) {
	fprintf(file, " %d", value);
}

void LogHexByte(FILE *file, int value) {
	fprintf(file, " %2x", value & 0xFF);
}