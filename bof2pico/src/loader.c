#include <windows.h>

WINBASEAPI LPVOID WINAPI KERNEL32$VirtualAlloc (LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
WINBASEAPI WINBOOL WINAPI KERNEL32$VirtualFree (LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType);
WINBASEAPI int __cdecl MSVCRT$vsnprintf(char * __restrict__ d,size_t n,const char * __restrict__ format,va_list arg);
WINBASEAPI size_t MSVCRT$strlen(const char *str);

char *output_buffer;

void bof_printf(char * format, ...) {
	int    len;
	char * temp;

	va_list args;

	/* figure out the length of our buffer */
	va_start(args, format);
	len  = MSVCRT$vsnprintf(NULL, 0, format, args);
	va_end(args);

	/* allocate our memory */
	temp = KERNEL32$VirtualAlloc(NULL, len + 1, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	if (temp == NULL)
		return;

	/* format everything */
	va_start(args, format);
	MSVCRT$vsnprintf(temp, len + 1, format, args);
	va_end(args);

	/* print it */
	size_t current_len = MSVCRT$strlen(output_buffer);
	size_t temp_len = MSVCRT$strlen(temp);
	for (int i = 0; i < temp_len && current_len + i < 10240; i++) {
		output_buffer[current_len + i] = temp[i];
	}

	/* free our memory and move on with our lives */
	KERNEL32$VirtualFree(temp, 0, MEM_RELEASE);
}

void BeaconOutput(int type, char * data, int len) {
	bof_printf("BeaconOut[%x]: %s\n", type, data);
}

VOID go(	IN PCHAR Buffer, IN ULONG Length);

char *_go(char *arg, size_t len) {
	output_buffer = KERNEL32$VirtualAlloc(0, 10240, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);

	go("", 0); // TODO you can't just pass the cmdline straight to beacon, needs some packing into the beacon argument format
	
	return output_buffer;
}
