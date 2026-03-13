from mythic_container.MythicCommandBase import *

class WhoamiArguments(TaskArguments):

	def __init__(self, command_line, **kwargs):
		super().__init__(command_line, **kwargs)
		self.args = []

	async def parse_arguments(self):
		if len(self.command_line) > 0:
			raise Exception("Whoami command takes no parameters.")

class WhoamiCommand(CommandBase):
	cmd = "whoami" # Name of the command
	help_cmd = "whoami" # Help information presented to the user
	argument_class = WhoamiArguments # The class used for processing & validating arguments
	description = "Get the username of the logged in user (ported to PICO format from TrustedSec's CS-Situational-Awareness-BOF repository)."
	needs_admin = False
	version = 1
	author = "@ofasgard"
	attackmapping = ["T1033"] # "Identify the primary user, currently logged in user, set of users that commonly uses a system, or whether a user is actively using the system"
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
