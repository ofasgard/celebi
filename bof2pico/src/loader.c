#include <windows.h>
#include "tcg.h"

WINBASEAPI size_t MSVCRT$strlen(const char *str);

void BeaconOutput(int type, char * data, int len) {
	dprintf("BeaconOut[%x]: %s", type, data); // TODO record output instead of debug printing
}

VOID go(	IN PCHAR Buffer, IN ULONG Length);

char *_go(char *arg, size_t len) {
	go(arg, len);
	return "SEE DPRINTF FOR OUTPUT"; // TODO output
}
