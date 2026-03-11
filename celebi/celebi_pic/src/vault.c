#include <windows.h>
#include "headers/celebi.h"

WINBASEAPI LPVOID WINAPI KERNEL32$VirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
WINBASEAPI BOOL WINAPI KERNEL32$VirtualFree(LPVOID lpAddress, SIZE_T dwSize, DWORD  dwFreeType);

WINBASEAPI int MSVCRT$strcmp(const char *string1, const char *string2);

DataVault new_vault() {
	DataVault vault = { 0 };
	
	vault.data = KERNEL32$VirtualAlloc(0, VAULT_INITIAL_SIZE, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	vault.data_size = VAULT_INITIAL_SIZE;
	vault.buffers = KERNEL32$VirtualAlloc(0, sizeof(DataBuffer) * 1024, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	vault.buffer_count = 0;
	
	return vault;
}

void extend_vault(DataVault *vault, size_t new_size) {
	char *new_data = KERNEL32$VirtualAlloc(0, new_size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	
	for (int i = 0; i < vault->data_size; i++) {
		new_data[i] = vault->data[i];
	}
	
	KERNEL32$VirtualFree(vault->data, 0, MEM_RELEASE);
	
	vault->data = new_data;
	vault->data_size = new_size;
}

void free_vault(DataVault *vault) {
	KERNEL32$VirtualFree(vault->data, 0, MEM_RELEASE);
	KERNEL32$VirtualFree(vault->buffers, 0, MEM_RELEASE);
}

void add_to_vault(DataVault *vault, char *name, char *buf, size_t buflen) {
	// Find the "end" of the current data in the vault.
	size_t offset = 0;
	
	for (int i = 0; i < vault->buffer_count; i++) {
		offset += vault->buffers[i].buffer_size;
	}
	
	// Check if we have enough space to simply perform a copy.
	if ((offset + buflen) > vault->data_size) {
		// If not, extend the vault until it is big enough.
		extend_vault(vault, vault->data_size + (buflen * 2));
	}
	
	// Perform the copy.
	for(int i = 0; i < buflen; i++) {
		vault->data[offset + i] = buf[i];
	}
	
	// Create a new DataBuffer to track this new object.
	DataBuffer databuf = { 0 };
	databuf.name = clone_str(name);
	databuf.buffer_offset = offset;
	databuf.buffer_size = buflen;
	
	// Add it to the vault.
	vault->buffers[vault->buffer_count] = databuf;
	vault->buffer_count++;
}

BOOL retrieve_from_vault(DataVault *vault, DataBuffer *out, char *key) {
	for (int i = 0; i < vault->buffer_count; i++) {
		if (MSVCRT$strcmp(key, vault->buffers[i].name) == 0) {
			out = &vault->buffers[i];
			return TRUE;
		}
	}

	return FALSE;
}

char *resolve_databuffer(DataVault *vault, DataBuffer *databuf) {
	return &(vault->data[databuf->buffer_offset]);
}

