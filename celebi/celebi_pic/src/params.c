#include <windows.h>
#include "headers/celebi.h"

WINBASEAPI LPVOID WINAPI KERNEL32$VirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
WINBASEAPI BOOL WINAPI KERNEL32$VirtualFree(LPVOID lpAddress, SIZE_T dwSize, DWORD  dwFreeType);

WINBASEAPI size_t MSVCRT$strlen(const char *str);

/*
 *
 * PACK
 *
*/

void pack_char(char *buf, int *offset, char paydata) {
	buf[*offset] = paydata;
	*offset += 1;
}

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

/*
 *
 * UNPACK
 *
*/

char unpack_char(char *buf, int *offset) {
	// Unpacks a single byte at the current offset.
	// The offset parameter is updated to the new position.
	
	char byte = buf[*offset];
	*offset += 1;
	return byte;
}

int unpack_int(char *buf, int *offset) {
	// Unpacks an integer at the current offset.
	// The offset parameter is updated to the end of the integer.
	
	int *int_ptr = (int *) &(buf[*offset]);
	*offset += sizeof(int);
	return *int_ptr;
}

unsigned int unpack_uint(char *buf, int *offset) {
	// Unpacks an unsigned integer at the current offset.
	// The offset parameter is updated to the end of the integer.
	
	unsigned int *int_ptr = (unsigned int *) &(buf[*offset]);
	*offset += sizeof(unsigned int);
	return *int_ptr;
}

char *unpack_str(char *buf, int *offset) {
	// Unpacks a string at the current offset by calculating its length and copying it over.
	// The offset parameter is updated to the end of the string.
	
	char *str_ptr = &(buf[*offset]);
	int str_len = MSVCRT$strlen(str_ptr);
	
	char *unpacked_str = KERNEL32$VirtualAlloc(0, str_len > 0 ? str_len : 10, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	append_str(unpacked_str, str_ptr);
	
	*offset += str_len + 1;
	return unpacked_str;
}

/*
 *
 * PARAMS
 *
*/

void unpack_params(char *enc_params, char *key, AgentParams *params) {
	// Takes the packed strings patched in by the linker, deobfuscates them, and unpacks it into an AgentParams struct.
	char *raw_params = KERNEL32$VirtualAlloc(0, PARAM_BUFFER_LEN, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	for (int i = 0; i < PARAM_BUFFER_LEN; i++) {
		raw_params[i] = enc_params[i] ^ key[i % XORKEY_LEN];
	}
	
	int offset = 0;
	
	params->payload_uuid = unpack_str(raw_params, &offset);
	params->callback_host = unpack_str(raw_params, &offset);
	params->callback_port = unpack_int(raw_params, &offset);
	params->callback_https = unpack_int(raw_params, &offset);
	params->callback_uri = unpack_str(raw_params, &offset);
	
	KERNEL32$VirtualFree(raw_params, 0, MEM_RELEASE);
}

void free_params(AgentParams *params) {
	if (params->payload_uuid != NULL) { KERNEL32$VirtualFree(params->payload_uuid, 0, MEM_RELEASE); }
	if (params->callback_host != NULL) { KERNEL32$VirtualFree(params->callback_host, 0, MEM_RELEASE); }
	if (params->callback_uri != NULL) { KERNEL32$VirtualFree(params->callback_uri, 0, MEM_RELEASE); }
}
