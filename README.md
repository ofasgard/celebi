# Celebi

*Crystal + Mythic = Celebi*

A WIP Mythic agent that uses Crystal Palace to build its payloads.

Current features:

- Performs a plaintext checkin with the specified C2 server via HTTP(S)
- Supports the `callback_host` and `callback_port` parameters to specify the C2 listener
- Supports the `post_uri` parameter to specify the URI for checking in

Current limitations:

- Only performs checkin (no tasking or commands)
- Only communicates using plaintext (no AES256)
- Only supports the http C2 profile
- Ignores most C2 profile parameters
- Doesn't gather any information about the target host
- Completely opsec unsafe: no sleep masking, no obfuscation logic, no tradecraft (yet!)

Currently working on:

- Implement tasking messages
- Implement an exit command
- Implement host recon during checkin

Longterm goals:

- Fully implement parameters from the http C2 profile
- Implement AES256 traffic encryption
- Implement PICO-based obfuscation
- Implement configuration parameters to swap obfuscation PICOs
- Implement a command to swap obfuscation PICOs at runtime

To be clear, that means this agent **only performs checkin**. As of right now, that's it. It doesn't receive tasking and it doesn't implement any commands, not even an exit command. It is an unfinished WIP, and it is also not opsec safe. Please don't try to use it in a real red team engagement.

> This project is released strictly for educational purposes, and is intended solely for use by authorised parties performing legitimate security research and red team assessments. My only intention is to share my work for the purposes of uplifting security.

## Installation

1. Clone the repository and copy both `celebi` and `celebi_translator` to your `Mythic/InstalledServices` folder.
2. Add them both to your docker-compose file: `mythic-cli add celebi` and `mythic-cli add celebi_translator`.
3. Build both containers: `mythic-cli build celebi` and `mythic-cli build celebi_translator`.
4. Build payloads for Celebi using the http C2 profile. You can use HTTP or HTTPS, but make sure that `AESPSK` is set to "none". 

## Acknowledgements

Thanks to:

- [Raphael Mudge](https://tradecraftgarden.org/crystalpalace.html) for Crystal Palace and LibTCG.
- [@pard0p](https://github.com/pard0p/LibWinHttp) for the LibWinHttp library that made implementing messaging much less of a headache.
- [Cody Thomas](https://github.com/its-a-feature) for Mythic (and excellent documentation!)
- [Leonardo Tamiano ](https://blog.leonardotamiano.xyz/tech/base64/) for a nice self-contained base64 implementation that plays nicely with PIC.
