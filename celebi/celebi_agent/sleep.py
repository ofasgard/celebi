from mythic_container.MythicCommandBase import *
from mythic_container.MythicRPC import *

class SleepArguments(TaskArguments):

	def __init__(self, command_line, **kwargs):
		super().__init__(command_line, **kwargs)
		self.args = [
			CommandParameter(
				name="interval",
				cli_name="interval",
				display_name="interval",
				type=ParameterType.Number,
				parameter_group_info=[ ParameterGroupInfo(required=True, ui_position=0) ]
			)
		]
		
	async def parse_arguments(self):
		if len(self.command_line) == 0:
			raise Exception("Please provide the sleep interval.")
		if self.command_line[0] != "{":
			raise Exception("Require JSON blob, but got raw command line.")
		
		self.load_args_from_json_string(self.command_line)
		
		if self.get_arg("interval") > 43200:
			raise Exception("Maximum sleep interval is 12 hours (43200 seconds).")
		
class SleepCommand(CommandBase):
	cmd = "sleep" # Name of the command
	help_cmd = "sleep [interval]" # Help information presented to the user
	argument_class = SleepArguments # The class used for processing & validating arguments
	description = "Adjust how long the agent sleeps before communicating with the C2 server."
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
