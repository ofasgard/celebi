#include <windows.h>

#define MAX_INPUT_SIZE  10240
#define MAX_OUTPUT_SIZE 10240

WINBASEAPI LPVOID WINAPI KERNEL32$VirtualAlloc (LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
WINBASEAPI WINBOOL WINAPI KERNEL32$VirtualFree (LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType);
WINBASEAPI int __cdecl MSVCRT$vsnprintf(char * __restrict__ d,size_t n,const char * __restrict__ format,va_list arg);
WINBASEAPI size_t MSVCRT$strlen(const char *str);
WINBASEAPI char *MSVCRT$strtok(char *strToken, const char *strDelimit);

char *output_buffer;

void pack_uint(char *buf, int *offset, unsigned int paydata) {
	for (int i = 0; i < sizeof(unsigned int); i++) {
		buf[*offset] = ((char *) &paydata)[i];
		*offset += 1;
	}
}

void pack_string(char *buf, int *offset, char *paydata) {
	if (paydata != NULL) {
		int len = MSVCRT$strlen(paydata);
		for (int i = 0; i < len; i++) {
			buf[*offset] = paydata[i];
			*offset += 1;
		}
	}
	
	// Add the null byte. If paydata is NULL, this results in a zero-length string.
	buf[*offset] = 0;
	*offset += 1;
}

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
	for (int i = 0; i < temp_len && current_len + i < MAX_OUTPUT_SIZE; i++) {
		output_buffer[current_len + i] = temp[i];
	}

	/* free our memory and move on with our lives */
	KERNEL32$VirtualFree(temp, 0, MEM_RELEASE);
}

void bof_process_args(char *arg, char *bof_args, int *bof_args_len) {
	char *token = MSVCRT$strtok(arg, " ");
	
	while (token != NULL) {
		size_t token_len = MSVCRT$strlen(token);
		pack_uint(bof_args, bof_args_len, token_len+1);
		pack_string(bof_args, bof_args_len, token);
		token = MSVCRT$strtok(NULL, " ");
	}
}

void BeaconOutput(int type, char * data, int len) {
	bof_printf("BeaconOut[%x]: %s\n", type, data);
}

VOID go(	IN PCHAR Buffer, IN ULONG Length);

char *_go(char *arg, size_t len) {
	output_buffer = KERNEL32$VirtualAlloc(0, MAX_OUTPUT_SIZE, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	
	if (arg == NULL || len == 0) {
		go("", 0); // Run with no arguments.
	} else {
		char *args_buffer = KERNEL32$VirtualAlloc(0, MAX_INPUT_SIZE, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
		int args_len = 0;
		
		bof_process_args(arg, args_buffer, &args_len);
		go(args_buffer, args_len); 
		KERNEL32$VirtualFree(args_buffer, 0, MEM_RELEASE);
	}
	
	return output_buffer;
}
