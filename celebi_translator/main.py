import mythic_container
import asyncio

import json, base64
from mythic_container.TranslationBase import *

MESSAGE_TYPE_CHECKIN = 1
MESSAGE_TYPE_TASKING = 2
MESSAGE_TYPE_POST    = 3

class CelebiTranslation(TranslationContainer):
    name = "celebi_translator"
    description = "Translator used by Celebi to serialize/deserialize message data in a custom format."
    author = "@ofasgard"

    async def generate_keys(self, inputMsg: TrGenerateEncryptionKeysMessage) -> TrGenerateEncryptionKeysMessageResponse:
        # Currently unused.
        response = TrGenerateEncryptionKeysMessageResponse(Success=True)
        response.DecryptionKey = b""
        response.EncryptionKey = b""
        return response

    async def translate_to_c2_format(self, inputMsg: TrMythicC2ToCustomMessageFormatMessage) -> TrMythicC2ToCustomMessageFormatMessageResponse:
        # The C2 is talking to the agent.
        response = TrMythicC2ToCustomMessageFormatMessageResponse(Success=True)
        
        if inputMsg.Message["action"] == "checkin":
            serialized_reply = self.serialize_checkin_reply(inputMsg.Message)
            response.Message = base64.b64encode(serialized_reply)
        if inputMsg.Message["action"] == "get_tasking":
            serialized_reply = self.serialize_tasking_reply(inputMsg.Message)
            response.Message = base64.b64encode(serialized_reply)
            
        return response

    async def translate_from_c2_format(self, inputMsg: TrCustomMessageToMythicC2FormatMessage) -> TrCustomMessageToMythicC2FormatMessageResponse:
        # The agent is talking to the C2.
        response = TrCustomMessageToMythicC2FormatMessageResponse(Success=True)
        
        if inputMsg.Message[0] == MESSAGE_TYPE_CHECKIN:
            response.Message = self.deserialize_checkin_request(inputMsg.UUID, inputMsg.Message)
            return response
        if inputMsg.Message[0] == MESSAGE_TYPE_TASKING:
            response.Message = self.deserialize_tasking_request(inputMsg.Message)
            return response
        if inputMsg.Message[0] == MESSAGE_TYPE_POST:
            response.Message = self.deserialize_post_request(inputMsg.Message) 
            return response
        
        raise Exception("UNRECOGNISED INPUT MESSAGE TYPE: {}".format(inputMsg.Message))

    def deserialize_checkin_request(self, payload_uuid, packed_msg):
        data = {}
        data["action"] = "checkin"
        data["uuid"] = payload_uuid
        
        offset = 1
        
        # Parse PID
        pid_raw = packed_msg[offset:offset+4]
        data["pid"] = int.from_bytes(pid_raw, "little", signed=False)
        offset += 4
        
        # Parse username
        data["user"] = ""
        for byte in packed_msg[offset:]:
            if byte == 0x00:
                break
            data["user"] += chr(byte)
            offset += 1
        
        offset +=1 # terminator byte
        
        # Parse hostname
        data["host"] = ""
        for byte in packed_msg[offset:]:
            if byte == 0x00:
                break
            data["host"] += chr(byte)
            offset += 1
        
        offset +=1 # terminator byte
        
        # Parse domain
        data["domain"] = ""
        for byte in packed_msg[offset:]:
            if byte == 0x00:
                break
            data["domain"] += chr(byte)
            offset += 1
        
        offset +=1 # terminator byte
        
        # Hardcoded parameters
        data["architecture"] = "x64"
        data["os"] = "Windows"
        
        return data
        
    def deserialize_tasking_request(self, packed_msg):
        data = {}
        data["action"] = "get_tasking"
        data["tasking_size"] = packed_msg[1]
        return data
        
    def deserialize_post_request(self, packed_msg):
        data = {}
        data["action"] = "post_response"
        
        response = {}
        offset = 1
        
        # Parse task ID
        response["task_id"] = ""
        for byte in packed_msg[offset:]:
            if byte == 0x00:
                break
            response["task_id"] += chr(byte)
            offset += 1
        
        offset +=1 # terminator byte   
        
        # Parse task output
        response["user_output"] = ""
        for byte in packed_msg[offset:]:
            if byte == 0x00:
                break
            response["user_output"] += chr(byte)
            offset += 1
        
        offset +=1 # terminator byte   
        
        # Parse task status
        response["status"] = ""
        for byte in packed_msg[offset:]:
            if byte == 0x00:
                break
            response["status"] += chr(byte)
            offset += 1
        
        offset +=1 # terminator byte           
        
        data["responses"] = [response]
        return data

    def serialize_checkin_reply(self, msg):
        output = bytearray()
        
        output.append(MESSAGE_TYPE_CHECKIN)
        
        output.extend(msg["id"].encode())
        output.append(0)
        
        output.extend(msg["status"].encode())
        output.append(0)
        
        return bytes(output)
        
    def serialize_tasking_reply(self, msg):
        output = bytearray()
        
        output.append(MESSAGE_TYPE_TASKING)
        
        task_count = len(msg["tasks"])
        output.extend(task_count.to_bytes(1, "big"))
        
        for task in msg["tasks"]:
                output.extend(task["id"].encode())
                output.append(0)
                
                output.extend(task["command"].encode())
                output.append(0)
                
                output.extend(task["parameters"].encode())
                output.append(0)
                
                rounded_timestamp = int(task["timestamp"])
                output.extend(rounded_timestamp.to_bytes(4, "big"))
        
        return bytes(output)

mythic_container.mythic_service.start_and_run_forever()
