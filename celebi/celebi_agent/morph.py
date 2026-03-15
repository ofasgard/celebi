from mythic_container.MythicCommandBase import *
from mythic_container.MythicRPC import *

import base64, json

class MorphArguments(TaskArguments):

	def __init__(self, command_line, **kwargs):
		super().__init__(command_line, **kwargs)
		self.args = [
			CommandParameter(
				name="command",
				cli_name="command",
				display_name="command",
				type=ParameterType.String,
				parameter_group_info=[ ParameterGroupInfo(required=True, ui_position=0) ]
			),
			CommandParameter(
				name="pico_name",
				cli_name="pico_name",
				display_name="pico_name",
				type=ParameterType.String,
				parameter_group_info=[ ParameterGroupInfo(required=True, ui_position=1) ]
			)
		]

	async def parse_arguments(self):
		if len(self.command_line) == 0:
			raise Exception("Please provide command to morph and the name of a registered PICO.")
		if self.command_line[0] != "{":
			raise Exception("Require JSON blob, but got raw command line.")
		self.load_args_from_json_string(self.command_line)
		
class MorphCommand(CommandBase):
	cmd = "morph" # Name of the command
	help_cmd = "morph [command] [pico]" # Help information presented to the user
	argument_class = MorphArguments # The class used for processing & validating arguments
	description = "Replace a built-in command with a compatible PICO that you have uploaded via the `register` command."
	needs_admin = False
	version = 1
	author = "@ofasgard"
	attackmapping = ["T1620"] # "Allocating then executing payloads directly within the memory of the process"
	supported_ui_features = []
	attributes = CommandAttributes(
		builtin=True, # Is this command always compiled into this payload type?
		suggested_command=True, # Is this command preselected when building a payload?
	)

	async def create_go_tasking(self, taskData: PTTaskMessageAllData) -> PTTaskCreateTaskingMessageResponse:
		response = PTTaskCreateTaskingMessageResponse(
			TaskID=taskData.Task.ID,
			Success=True,
		)
		
		return response
