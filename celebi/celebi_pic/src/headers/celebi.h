#include "HTTP.h"

#define MESSAGE_TYPE_CHECKIN 1

typedef struct AgentParams {
	char *payload_uuid;
	char *callback_host;
	int callback_port;
	int callback_https;
	char *callback_uri;
} AgentParams;

typedef struct CheckinRequest {
	char *payload_uuid;
} CheckinRequest;

typedef struct CheckinReply {
	char action;
	char *callback_uuid;
	char *status;
} CheckinReply;

void append_str(char *string, char *append);
void base64_encode(const char *in, const unsigned long in_len, char *out);

char *generate_checkin_message(CheckinRequest *checkin);
void parse_checkin_reply(HttpResponse *response, CheckinReply *reply);
void free_checkin_reply(CheckinReply *reply);

char *unpack_str(char *raw_params, int *offset);
int unpack_int(char *raw_params, int *offset);
void unpack_params(char *raw_params, AgentParams *params);
void free_params(AgentParams *params);
