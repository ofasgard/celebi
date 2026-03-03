#include <windows.h>
#include "headers/celebi.h"

WINBASEAPI LPVOID WINAPI KERNEL32$VirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
WINBASEAPI BOOL WINAPI KERNEL32$VirtualFree(LPVOID lpAddress, SIZE_T dwSize, DWORD  dwFreeType);

WINBASEAPI size_t MSVCRT$strlen(const char *str);

char *unpack_str(char *raw_params, int *offset) {
	// Unpacks a string at the current offset by calculating its length and copying it over.
	// The offset parameter is updated to the end of the string.
	
	char *str_ptr = &(raw_params[*offset]);
	int str_len = MSVCRT$strlen(str_ptr);
	
	char *unpacked_str = KERNEL32$VirtualAlloc(0, str_len, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	append_str(unpacked_str, str_ptr);
	
	*offset += str_len + 1;
	return unpacked_str;
}

int unpack_int(char *raw_params, int *offset) {
	// Unpacks an integer at the current offset.
	// The offset parameter is updated to the end of the integer.
	
	int *int_ptr = (int *) &(raw_params[*offset]);
	*offset += sizeof(int);
	return *int_ptr;
}

void unpack_params(char *raw_params, AgentParams *params) {
	// Takes the packed strings patched in by the linker and unpacks it into an AgentParams struct.
	int offset = 0;
	
	params->payload_uuid = unpack_str(raw_params, &offset);
	params->callback_host = unpack_str(raw_params, &offset);
	params->callback_port = unpack_int(raw_params, &offset);
	params->callback_https = unpack_int(raw_params, &offset);
	params->callback_uri = unpack_str(raw_params, &offset);
}

void free_params(AgentParams *params) {
	if (params->payload_uuid != NULL) { KERNEL32$VirtualFree(params->payload_uuid, 0, MEM_RELEASE); }
	if (params->callback_host != NULL) { KERNEL32$VirtualFree(params->callback_host, 0, MEM_RELEASE); }
	if (params->callback_uri != NULL) { KERNEL32$VirtualFree(params->callback_uri, 0, MEM_RELEASE); }
}
