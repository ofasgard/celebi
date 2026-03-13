# Celebi

*Crystal + Mythic = Celebi*

A WIP Mythic agent for x64 Windows that uses Crystal Palace to build its payloads.

To be clear, this agent is an unfinished WIP, and it is also not opsec safe. Please don't try to use it in a real red team engagement.

Current features:

- Performs a plaintext checkin with the specified C2 server via HTTP(S).
- Supports the `callback_host` and `callback_port` parameters to specify the C2 listener.
- Supports the `post_uri` parameter to specify the URI for checking in.
- Supports the `exit` and `whoami` built-in commands.
- Supports a `register` and `unregister` command to upload or delete files from the agent's memory vault.
- Supports an `execute_pico` command to interpret a registered file as a Crystal Palace PICO and run it.

Current limitations:

- Only implements basic functionality
- Only supports x64 architectures
- Only communicates using plaintext (no AES256)
- Only supports the http C2 profile
- Ignores most C2 profile parameters
- Completely opsec unsafe: no sleep masking, no obfuscation logic, no tradecraft (yet!)
- Very little error handling, will probably crash if something unexpected happens
- The `register` command uses a very simple/naive "memory vault" implementation that endlessly grows as you register more files.
- Likewise, the `unregister` command literally just zeroes out the file and replaces its name with a dummy string.

Longterm goals:

- Fully implement parameters from the http C2 profile
- Implement AES256 traffic encryption
- Implement "core" obfuscation logic such as sleepmasking as a set of PICOs (default PICOs aren't opsec-safe, but you can swap them out for any PICO that follows the same convention!)
- Port over TrustedSec's situational awareness BOFs (GPL) and use them to provide some built-in PICO commands to the agent.
- Implement `execute_bof` and `execute_shellcode` commands as a supplement to the `execute_pico` command.
- Implement a `morph` command to hotswap a built-in PICO with one you uploaded with the `register` command
- Support other C2 profiles
- Include YARA rules for the "default" build of the agent

> This project is released strictly for educational purposes, and is intended solely for use by authorised parties performing legitimate security research and red team assessments. My only intention is to share my work for the purposes of uplifting security.

## Installation

1. Clone the repository and copy both `celebi` and `celebi_translator` to your `Mythic/InstalledServices` folder.
2. Add them both to your docker-compose file: `mythic-cli add celebi` and `mythic-cli add celebi_translator`.
3. Build both containers: `mythic-cli build celebi` and `mythic-cli build celebi_translator`.
4. Build payloads for Celebi using the http C2 profile. You can use HTTP or HTTPS, but make sure that `AESPSK` is set to "none". 

## Design

*Note that the design described here represents my aspirations for Celebi rather than its current state.*

The overall design goal of Celebi is to hardcode as little functionality as possible. Instead, we implement basic functionality such as sleep masking, information gathering, or command execution into a set of PICOs that are linked into the final implant. The PICOs that ship with Celebi by default are intended to "just work" without being opsec safe, but they can be replaced with your own custom Crystal Palace PICOs that implement the same interface.

The list of built-in PICOs currently includes:

- `checkin.c`: Performs basic information gathering about the target system and uses the collected data to enrich a `CheckinRequest` struct.
- `whoami.c`: A port of the whoami BOF from TrustedSec's CS-Situational-Awareness-BOF repository. Used by the `whoami` built-in command.

In addition, the design calls for the ability to change every aspect of the implant while it is running. Currently, you can do this with the `unregister` and `register` commands. For example, if you unregister `_builtin_whoami` and register a new PICO with the same name, the behaviour of the whoami command will change. This allows your agent to dynamically change its TTPs and behaviour without recompiling the underlying shellcode; I plan to implement a `morph` command to streamline this process.

I don't intend to write a large number of commands for this agent. Beyond basic convenience commands like `whoami` or `download`, the plan is for most of the agent's capabilities to be loaded *after* it starts running, in the form of BOFs or PICOs. I plan to provide support for both. Likewise, I don't plan to implement many (or maybe any) of Mythic's "optional" commands, because most of the implant's capabilities are intended to be loaded in remotely.

## Example: Executing a PICO

The function signature for a generic PICO that celebi knows how to execute is:

```c
typedef char *(*GENERIC_PICO)(char *cmdline);
```

Here's an example of a very simple PICO that implements this interface:

```c
#include <windows.h>
 
WINBASEAPI VOID WINAPI KERNEL32$OutputDebugStringA (LPCSTR lpOutputString);
 
char *go(char * arg) {
    KERNEL32$OutputDebugStringA(arg);
    return "printed a debug string!";
}
```

You can compile this into a 64-bit COFF using MinGW, and link it with a very simple linker script:

```text
x64:
	load "dbg_pico.o"
	make object
	export
```

Use Crystal Palace to do so:

```console
$ piclink linker.spec x64 dbg_pico.bin
```

Then you can use the `register` command to upload your PICO, and `execute_pico` to invoke it!


![A screenshot demonstrating executing a PICO with Celebi](docs/execute-pico-1.png)

![A screenshot demonstrating the successful execution of the PICO](docs/execute-pico-2.png)

## Acknowledgements

Thanks to:

- [Raphael Mudge](https://tradecraftgarden.org/crystalpalace.html) for Crystal Palace and LibTCG.
- [@pard0p](https://github.com/pard0p/LibWinHttp) for the LibWinHttp library that made implementing messaging much less of a headache.
- [Cody Thomas](https://github.com/its-a-feature) for Mythic (and excellent documentation!)
- [Leonardo Tamiano](https://blog.leonardotamiano.xyz/tech/base64/) for a nice self-contained base64 implementation that plays nicely with PIC.
- [TrustedSec](https://github.com/trustedsec/CS-Situational-Awareness-BOF) for some open-source BOFs which I ported into PICOs to implement some of celebi's built-in commands.
