#include <windows.h>
#include "../headers/celebi.h"

void go(CheckinRequest *req) {
	req->payload_uuid[0]= 0x41;
	req->payload_uuid[1]= 0x42;
	req->payload_uuid[2]= 0x43;
	req->payload_uuid[3]= 0x44;
}
