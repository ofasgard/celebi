#include <windows.h>

WINBASEAPI DWORD WINAPI KERNEL32$WaitForSingleObject(	HANDLE hHandle, DWORD dwMilliseconds);

void go(char *pic, int sleep_time, char *key, int keylen) {
	KERNEL32$WaitForSingleObject(((HANDLE)(LONG_PTR)-1), sleep_time * 1000);
}
