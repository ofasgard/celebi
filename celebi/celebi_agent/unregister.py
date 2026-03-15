from mythic_container.MythicCommandBase import *
from mythic_container.MythicRPC import *

import base64, json

class UnregisterArguments(TaskArguments):

	def __init__(self, command_line, **kwargs):
		super().__init__(command_line, **kwargs)
		self.args = [
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
			raise Exception("Please provide the name of a file to unregister.")
		if self.command_line[0] != "{":
			raise Exception("Require JSON blob, but got raw command line.")
		
		self.load_args_from_json_string(self.command_line)
		
		if len(self.get_arg("name")) == 0:
			raise Exception("You must provide a value for the name argument")
		
class UnregisterCommand(CommandBase):
	cmd = "unregister" # Name of the command
	help_cmd = "unregister [name]" # Help information presented to the user
	argument_class = UnregisterArguments # The class used for processing & validating arguments
	description = "Task the agent to clear a registered file from its mapped memory."
	needs_admin = False
	version = 1
	author = "@ofasgard"
	attackmapping = ["T1070"] # "Adversaries may delete or modify artifacts generated within systems to remove evidence of their presence"
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
