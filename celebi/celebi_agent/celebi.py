from mythic_container.PayloadBuilder import *
from mythic_container.MythicCommandBase import *
from mythic_container.MythicRPC import *

import subprocess
from urllib.parse import urlsplit

class CelebiAgent(PayloadType):
	name = "celebi"
	file_extension = "bin"
	agent_type = AgentType.Agent
	author = "@ofasgard"
	mythic_encrypts = False
	supported_os = [
		SupportedOS.Windows
	]
	semver = "0.1.7"
	note = "A PoC agent that uses Crystal Palace to build its payload."
	wrapped_payloads = []
	supports_dynamic_loading = True
	shellcode_format_options = ["Binary"]
	shellcode_bypass_options = ["None"]
	supports_multiple_c2_instances_in_build = False
	supports_multiple_c2_in_build = False
	build_parameters = [
	BuildParameter(name="debug", parameter_type=BuildParameterType.Boolean, default_value=False, description="Enable dprintf() debugging."),
	BuildParameter(name="exit_func", parameter_type=BuildParameterType.ChooseOne, choices=["process", "thread"], default_value="process", description="Use ExitProcess() or ExitThread() to exit.")
	]
	build_steps = []
	c2_profiles = ["http"]
	c2_parameter_deviations = {}
	translation_container = "celebi_translator"
	agent_path = pathlib.Path(".")
	agent_code_path = agent_path / "celebi_agent"
	agent_icon_path = agent_path / "icon.svg"

	async def build(self) -> BuildResponse:
		# This function gets called to create an instance of your payload.
		self.configure_pic()
		
		parameters = {
			"debug": self.get_parameter("debug"),
			"exit_func": self.get_parameter("exit_func")
		}
		self.build_pic(parameters)
		
		resp = BuildResponse(status=BuildStatus.Success)
		resp.payload = open("/Mythic/celebi_pic/out/main.bin", "rb").read()
		
		return resp
	
	def configure_pic(self):
		config = open("/Mythic/templates/config.spec", "r").read()
		c2params = self.c2info[0].get_parameters_dict()
		
		# Patch in the payload UUID.
		config = config.replace("### PAYLOAD_UUID ###", self.uuid)
		
		# Patch in the C2 server host.
		parsed_url = urlsplit(c2params["callback_host"])
		config = config.replace("### CALLBACK_HOST ###", parsed_url.hostname)
		
		# Patch in the C2 server port.
		config = config.replace("### CALLBACK_PORT ###", str(c2params["callback_port"]))
		
		## Patch in the C2 server HTTPS toggle.
		if "https" in parsed_url.scheme.lower():
				config = config.replace("### CALLBACK_HTTPS ###", "1")
		else:
				config = config.replace("### CALLBACK_HTTPS ###", "0")
		
		# Patch in the C2 server URI.
		uri = "/{}".format(c2params["post_uri"])
		config = config.replace("### CALLBACK_URI ###", uri)
		
		fd = open("/Mythic/celebi_pic/config.spec", "w")
		fd.write(config)
		fd.close()
	
	def build_pic(self, parameters):
		proc = subprocess.Popen(["make", "clean"], cwd="/Mythic/celebi_pic/")
		proc.wait()

		cflags = []
		if parameters["debug"] == True:
			cflags.append("-DCELEBI_DEBUG")
		if parameters["exit_func"] == "thread":
			cflags.append("-DCELEBI_EXIT_THREAD")
		
		if len(cflags) > 0:
			cflags_str = "CFLAGS=\"{}\"".format(" ".join(cflags))
			proc = subprocess.Popen(["make", "pic", cflags_str], stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd="/Mythic/celebi_pic/")
		else:
			proc = subprocess.Popen(["make", "pic"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd="/Mythic/celebi_pic/")
		proc.wait()
		
# 0.0.x = initial PoC, non-functional
# 0.1.x = pre-alpha, functional but incomplete
# 0.2.x = alpha, mostly complete, no expectation of stability
# 0.3.x = beta, feature complete, stable but needs testing
# 1.0.0 = stable
