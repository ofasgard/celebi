from mythic_container.MythicCommandBase import *
from mythic_container.MythicRPC import *

class ExecutePicoArguments(TaskArguments):

	def __init__(self, command_line, **kwargs):
		super().__init__(command_line, **kwargs)
		self.args = [
			CommandParameter(
				name="name",
				cli_name="name",
				display_name="name",
				type=ParameterType.String,
				parameter_group_info=[ ParameterGroupInfo(required=True, ui_position=0) ]
			),
			CommandParameter(
				name="pico_args",
				cli_name="pico_args",
				display_name="pico_args",
				type=ParameterType.String,
				parameter_group_info=[ ParameterGroupInfo(required=True, ui_position=1) ]
			)
		]
		
	async def parse_arguments(self):
		if len(self.command_line) == 0:
			raise Exception("Please provide the name of a loaded PICO and some arguments.") # TODO
		if self.command_line[0] != "{":
			raise Exception("Require JSON blob, but got raw command line.")
		self.load_args_from_json_string(self.command_line)
		
class ExecutePicoCommand(CommandBase):
	cmd = "execute_pico" # Name of the command
	help_cmd = "execute_pico [name] [args]" # Help information presented to the user
	argument_class = ExecutePicoArguments # The class used for processing & validating arguments
	description = "Invoke a PICO loaded into memory with the `register` command."
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
