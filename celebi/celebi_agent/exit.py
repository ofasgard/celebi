from mythic_payloadtype_container.PayloadBuilder import *
from mythic_payloadtype_container.MythicCommandBase import *

class ExitArguments(TaskArguments):

    def __init__(self, command_line, **kwargs):
        super().__init__(command_line, **kwargs)
        self.args = []

    async def parse_arguments(self):
        if len(self.command_line) > 0:
            raise Exception("Exit command takes no parameters.")

class ExitCommand(CommandBase):
    cmd = "exit" # Name of the command
    help_cmd = "exit" # Help information presented to the user
    argument_class = ExitArguments # The class used for processing & validating arguments
    description = "Tasks the agent to clean up resources and exit."
    needs_admin = False
    version = 1
    author = "@ofasgard"
    attackmapping = []
    supported_ui_features = ["callback_table:exit"]
    attributes = CommandAttributes(
        builtin=True, # Is this command always compiled into this payload type?
        suggested_command=True, # Is this command preselected when building a payload?
    )

    async def create_go_tasking(self, taskData: MythicCommandBase.PTTaskMessageAllData) -> MythicCommandBase.PTTaskCreateTaskingMessageResponse:
        response = MythicCommandBase.PTTaskCreateTaskingMessageResponse(
            TaskID=taskData.Task.ID,
            Success=True,
        )
        return response
