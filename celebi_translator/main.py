import mythic_container
import asyncio

import json, base64
from mythic_container.TranslationBase import *

MESSAGE_TYPE_CHECKIN = 1

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
            
        return response

    async def translate_from_c2_format(self, inputMsg: TrCustomMessageToMythicC2FormatMessage) -> TrCustomMessageToMythicC2FormatMessageResponse:
        # The agent is talking to the C2.
        response = TrCustomMessageToMythicC2FormatMessageResponse(Success=True)
        
        if inputMsg.Message[0] == MESSAGE_TYPE_CHECKIN:
        	    response.Message = self.deserialize_checkin_request(inputMsg.UUID, inputMsg.Message)
        
        return response

    def deserialize_checkin_request(self, payload_uuid, packed_msg):
        # For now, checkin request from agent is literally just UUID and message type.
        # We don't use the message body to build the request yet, but we will when the agent actually sends data about the host.
        data = {}
        data["action"] = "checkin"
        data["uuid"] = payload_uuid
        return data

    def serialize_checkin_reply(self, msg):
        output = bytearray()
        
        output.extend(msg["id"].encode())
        output.append(0)
        output.extend(msg["status"].encode())
        output.append(0)
        
        return bytes(output)

mythic_container.mythic_service.start_and_run_forever()
