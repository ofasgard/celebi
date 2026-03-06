# Celebi

*Crystal + Mythic = Celebi*

A WIP Mythic agent that uses Crystal Palace to build its payloads.

To be clear, this agent currently **only performs checkin and tasking**. As of right now, that's it. The only command it implements is `exit`. It is an unfinished WIP, and it is also not opsec safe. Please don't try to use it in a real red team engagement.

Current features:

- Performs a plaintext checkin with the specified C2 server via HTTP(S)
- Supports the `callback_host` and `callback_port` parameters to specify the C2 listener
- Supports the `post_uri` parameter to specify the URI for checking in
- Supports the `exit` command

Current limitations:

- Only implements basic functionality
- Only communicates using plaintext (no AES256)
- Only supports the http C2 profile
- Ignores most C2 profile parameters
- Completely opsec unsafe: no sleep masking, no obfuscation logic, no tradecraft (yet!)

Longterm goals:

- Fully implement parameters from the http C2 profile
- Implement AES256 traffic encryption
- Implement "core" obfuscation logic such as sleepmasking as a set of PICOs (default PICOs aren't opsec-safe, but you can swap them out for any PICO that follows the same convention!)
- Implement some basic convenience commands such as `getuid`, again as swappable PICOs
- Implement a `morph` command to swap obfuscation PICOs at runtime
- Implement a `load` command to load both PICO and BOF capabilities

> This project is released strictly for educational purposes, and is intended solely for use by authorised parties performing legitimate security research and red team assessments. My only intention is to share my work for the purposes of uplifting security.

## Installation

1. Clone the repository and copy both `celebi` and `celebi_translator` to your `Mythic/InstalledServices` folder.
2. Add them both to your docker-compose file: `mythic-cli add celebi` and `mythic-cli add celebi_translator`.
3. Build both containers: `mythic-cli build celebi` and `mythic-cli build celebi_translator`.
4. Build payloads for Celebi using the http C2 profile. You can use HTTP or HTTPS, but make sure that `AESPSK` is set to "none". 

## Design

The overall design goal of Celebi is to hardcode as little functionality as possible. Instead, we implement basic functionality such as sleep masking, information gathering, or command execution into a set of PICOs that are linked into the final implant. The PICOs that ship with Celebi by default are intended to "just work" without being opsec safe, but they can be replaced with your own custom Crystal Palace PICOs that implement the same interface.

The list of built-in PICOs currently includes:

- `checkin.c`: Performs basic information gathering about the target system and uses the collected data to enrich a `CheckinRequest` struct.

In addition, the design calls for the ability to change every aspect about the implant while it is running. This is currently not implemented, but will come in the form of a command that allows you to replace any of the built-in PICOs with an uploaded PICO at runtime. This allows your agent to dynamically change its TTPs and behaviour without recompiling the underlying shellcode.

I don't intend to write a large number of commands for this agent. Beyond basic convenience commands like `getuid` or `download`, the plan is for most of the agent's capabilities to be loaded *after* it starts running, in the form of BOFs or PICOs. I plan to provide support for both.

## Acknowledgements

Thanks to:

- [Raphael Mudge](https://tradecraftgarden.org/crystalpalace.html) for Crystal Palace and LibTCG.
- [@pard0p](https://github.com/pard0p/LibWinHttp) for the LibWinHttp library that made implementing messaging much less of a headache.
- [Cody Thomas](https://github.com/its-a-feature) for Mythic (and excellent documentation!)
- [Leonardo Tamiano ](https://blog.leonardotamiano.xyz/tech/base64/) for a nice self-contained base64 implementation that plays nicely with PIC.
