#include <windows.h>

void xorify(char *out, char *in, size_t buflen, char *key, size_t keylen) {
	for (int i = 0; i < buflen; i++) {
		out[i] = in[i] ^ key[i % keylen];
	}
}

void go(char *vault, int vault_size, char *key, int keylen) {
	xorify(vault, vault, vault_size, key, keylen);
}
