# bof2pico

Helper utility for converting a compiled BOF into a PICO that celebi knows how to execute. Adapted from [Simple BOF Runner](https://tradecraftgarden.org/simplebof.html) by Raphael Mudge, and reproduces the `bofapi.c` file in its entirety. 

Note that the current version prints output using `dprintf()`, so the output won't be returned to the agent. I'm planning to make it return output properly at some point, but until then this is just for testing purposes.

Usage:

```sh
$ make all
$ cpl link ./linker.spec /path/to/get_session_info.x64.o ./get_session_info.x64.pico
```

Requires Crystal Palace.
