#include "headers/celebi.h"

WINBASEAPI LPVOID WINAPI KERNEL32$VirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
WINBASEAPI BOOL WINAPI KERNEL32$VirtualFree(LPVOID lpAddress, SIZE_T dwSize, DWORD  dwFreeType);

WINBASEAPI size_t MSVCRT$strlen(const char *str);

const int UNBASE64[] = {
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 0-11
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 12-23
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 24-35
-1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, // 36-47
52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -2, // 48-59
-1,  0, -1, -1, -1,  0,  1,  2,  3,  4,  5,  6, // 60-71
7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, // 72-83
19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, // 84-95
-1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, // 96-107
37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, // 108-119
49, 50, 51, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 120-131
};

const char BASE64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void append_str(char *string, char *append) {
	size_t current_len = MSVCRT$strlen(string);
	size_t append_len = MSVCRT$strlen(append);
	
	for(int i = 0; i < append_len; i++) {
		string[current_len + i] = append[i];
	}
}

void base64_encode(const char *in, const unsigned long in_len, char *out) {
	// Base64 implementation taken from here: https://blog.leonardotamiano.xyz/tech/base64/
	
	int in_index = 0;
	int out_index = 0;

	while (in_index < in_len) {
		// process group of 24 bit

		// first 6-bit
		out[out_index++] = BASE64[ (in[in_index] & 0xFC) >> 2 ];

		if ((in_index + 1) == in_len) {
			// padding case n.1
			//
			// Remaining bits to process are the right-most 2 bit of on the
			// last byte of input. we also need to add two bytes of padding
			out[out_index++] = BASE64[ ((in[in_index] & 0x3) << 4) ];
			out[out_index++] = '=';
			out[out_index++] = '=';
			break;
		}

		// second 6-bit
		out[out_index++] = BASE64[ ((in[in_index] & 0x3) << 4) | ((in[in_index+1] & 0xF0) >> 4) ];

		if ((in_index + 2) == in_len) {
			// padding case n.2
			//
			// Remaining bits to process are the right most 4 bit on the
			// last byte of input. We also need to add a single byte of
			// padding.
			out[out_index++] = BASE64[ ((in[in_index + 1] & 0xF) << 2) ];
			out[out_index++] = '=';
			break;
		}

		// third 6-bit
		out[out_index++] = BASE64[ ((in[in_index + 1] & 0xF) << 2) | ((in[in_index + 2] & 0xC0) >> 6) ];

		// fourth 6-bit
		out[out_index++] = BASE64[ in[in_index + 2] & 0x3F ];

		in_index += 3;
	}

	out[out_index] = '\0';
	return;
}

int base64_decode(const char *in, const unsigned long in_len, char *out) {
  // Base64 implementation taken from here: https://blog.leonardotamiano.xyz/tech/base64/
  
  int in_index = 0;
  int out_index = 0;
  char first, second, third, fourth;

  while( in_index < in_len ) {
    // check if next 4 byte of input is valid base64

    for (int i = 0; i < 4; i++) {
      if (((int)in[in_index + i] > 131) || UNBASE64[ (int) in[in_index + i] ] == -1) {
	    return -1;
      }
    }

    // extract all bits and reconstruct original bytes
    first = UNBASE64[ (int) in[in_index] ];
    second = UNBASE64[ (int) in[in_index + 1] ];
    third = UNBASE64[ (int) in[in_index + 2] ];
    fourth = UNBASE64[ (int) in[in_index + 3] ];

    // reconstruct first byte
    out[out_index++] = (first << 2) | ((second & 0x30) >> 4);

    // reconstruct second byte
    if (in[in_index + 2] != '=') {
      out[out_index++] = ((second & 0xF) << 4) | ((third & 0x3C) >> 2);
    }

    // reconstruct third byte
    if (in[in_index + 3] != '=') {
      out[out_index++] = ((third & 0x3) << 6) | fourth;
    }

    in_index += 4;
  }

  out[out_index] = '\0';
  return 0;
}
