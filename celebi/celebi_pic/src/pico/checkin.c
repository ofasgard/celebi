#include <windows.h>
#include "../headers/celebi.h"

WINBASEAPI LPVOID WINAPI KERNEL32$VirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
WINBASEAPI BOOL WINAPI KERNEL32$VirtualFree(LPVOID lpAddress, SIZE_T dwSize, DWORD  dwFreeType);

WINBASEAPI BOOL ADVAPI32$GetUserNameA(LPCSTR lpBuffer, LPDWORD pcbBuffer);

void go(CheckinRequest *req) {
	BOOL result;
	DWORD len;
	
	char *username = KERNEL32$VirtualAlloc(0, 256, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	result = ADVAPI32$GetUserNameA(username, &len);
	if (result == TRUE) {
		req->username = username;
	} else {
		KERNEL32$VirtualFree(username, 0, MEM_RELEASE);
	}
}
