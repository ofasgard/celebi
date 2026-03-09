from mythic_container.MythicCommandBase import *

class GetuidArguments(TaskArguments):

	def __init__(self, command_line, **kwargs):
		super().__init__(command_line, **kwargs)
		self.args = []

	async def parse_arguments(self):
		if len(self.command_line) > 0:
			raise Exception("Getuid command takes no parameters.")

class GetuidCommand(CommandBase):
	cmd = "getuid" # Name of the command
	help_cmd = "getuid" # Help information presented to the user
	argument_class = GetuidArguments # The class used for processing & validating arguments
	description = "Get the username of the logged in user."
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
