#include <windows.h>
#include "headers/celebi.h"
#include "headers/tcg.h"

WINBASEAPI HMODULE WINAPI KERNEL32$LoadLibraryA(LPCSTR lpLibFileName);
WINBASEAPI LPVOID WINAPI KERNEL32$GetProcAddress(HMODULE hModule, LPCSTR lpProcName);
WINBASEAPI LPVOID WINAPI KERNEL32$VirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
WINBASEAPI BOOL WINAPI KERNEL32$VirtualFree(LPVOID lpAddress, SIZE_T dwSize, DWORD  dwFreeType);

char __CHECKIN_PICO__[0] __attribute__((section("pico_checkin")));
char __GETUID_PICO__[0] __attribute__((section("pico_getuid")));

char * find_checkin_pico() {
    return (char *)&__CHECKIN_PICO__;
}

char * find_getuid_pico() {
    return (char *)&__GETUID_PICO__;
}

void load_builtin_picos(DataVault *vault) {
	_EMBEDDED_PICO *checkin = (_EMBEDDED_PICO *) find_checkin_pico();
	add_to_vault(vault, "_builtin_checkin", checkin->value, checkin->length);
	for (int i = 0; i < checkin->length; i++) { checkin->value[i] = 0; }
	
	_EMBEDDED_PICO *getuid = (_EMBEDDED_PICO *) find_getuid_pico();
	add_to_vault(vault, "_builtin_getuid", getuid->value, getuid->length);
	for (int i = 0; i < getuid->length; i++) { getuid->value[i] = 0; }
}

ResolvedPico resolve_loaded_pico(DataVault *vault, char *key) {
	WIN32FUNCS funcs;
	funcs.LoadLibraryA = (__typeof__(LoadLibraryA) *) KERNEL32$LoadLibraryA;
	funcs.GetProcAddress = (__typeof__(GetProcAddress) *) KERNEL32$GetProcAddress;
	funcs.VirtualAlloc = (__typeof__(VirtualAlloc) *) KERNEL32$VirtualAlloc;
	funcs.VirtualFree = (__typeof__(VirtualFree) *) KERNEL32$VirtualFree;

	DataBuffer databuf = { 0 };
	retrieve_from_vault(vault, &databuf, key);
	char *buf = resolve_databuffer(vault, &databuf);
	
	// Note that buf is a pointer into the vault, which might become invalid if the vault is extended (reallocated).
	// It's the caller's responsibility to resolve the pico, invoke it, then free it BEFORE calling add_to_vault() again.
	
	ResolvedPico pico = { 0 };
	pico.codelen = PicoCodeSize(buf);
	pico.code = KERNEL32$VirtualAlloc(NULL, pico.codelen, MEM_RESERVE|MEM_COMMIT|MEM_TOP_DOWN, PAGE_EXECUTE_READWRITE); // TODO allocate RW and reprotect after calling PicoLoad()
	pico.datalen = PicoDataSize(buf);
	pico.data = KERNEL32$VirtualAlloc(NULL, pico.datalen, MEM_RESERVE|MEM_COMMIT|MEM_TOP_DOWN, PAGE_READWRITE);
	
	PicoLoad((IMPORTFUNCS *) &funcs, buf, pico.code, pico.data);
	pico.entrypoint = PicoEntryPoint(buf, pico.code);
	
	return pico;
}

void free_resolved_pico(ResolvedPico *pico) {
	KERNEL32$VirtualFree(pico->code, 0, MEM_RELEASE);
	KERNEL32$VirtualFree(pico->data, 0, MEM_RELEASE);
}
