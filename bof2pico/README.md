# bof2pico

Helper utility for converting a compiled BOF into a PICO that celebi knows how to execute. Adapted from [Simple BOF Runner](https://tradecraftgarden.org/simplebof.html) by Raphael Mudge, and reproduces the `bofapi.c` file in its entirety. 

Note that the current version drops arguments, so it only works with BOFs that don't require any (i.e. `whoami` or `get_session_info`). I'm planning to implement support for BOF-style arguments soon!

Usage:

```sh
$ make all
$ cpl link ./linker.spec /path/to/get_session_info.x64.o ./get_session_info.x64.pico
```

Requires Crystal Palace.
