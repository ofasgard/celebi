from mythic_container.MythicCommandBase import *
from mythic_container.MythicRPC import *

import base64, json

class RegisterArguments(TaskArguments):

	def __init__(self, command_line, **kwargs):
		super().__init__(command_line, **kwargs)
		self.args = [
			CommandParameter(
				name="file",
				cli_name="file",
				display_name="file",
				type=ParameterType.File,
				parameter_group_info=[ ParameterGroupInfo(required=True) ]
			),
			CommandParameter(
				name="name",
				cli_name="name",
				display_name="name",
				type=ParameterType.String,
				parameter_group_info=[ ParameterGroupInfo(required=True) ]
			)
		]

	async def parse_arguments(self):
		if len(self.command_line) == 0:
			raise Exception("Please provide the path to a file to register.")
		if self.command_line[0] != "{":
			raise Exception("Require JSON blob, but got raw command line.")
		self.load_args_from_json_string(self.command_line)
		
class RegisterCommand(CommandBase):
	cmd = "register" # Name of the command
	help_cmd = "register" # Help information presented to the user
	argument_class = RegisterArguments # The class used for processing & validating arguments
	description = "Upload a file (via modal popup) to the agent and load it into mapped memory."
	needs_admin = False
	version = 1
	author = "@ofasgard"
	attackmapping = []
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
