# bof2pico

Helper utility for converting a compiled BOF into a PICO that celebi is able to execute. Adapted from [Simple BOF Runner](https://tradecraftgarden.org/simplebof.html) by Raphael Mudge, and reproduces the `bofapi.c` file in its entirety.

Usage:

```sh
$ make all
$ cpl link ./bof2pico.spec /path/to/somebof.x64.o ./somebof.x64.pico '$BOF_ARGS=00'
```

## Passing Arguments

If you want to pass arguments, you will need to construct your arguments in the BOF argument format and pass them in the `BOF_ARGS` variable. I recommend using the generator from [COFFLoader](https://github.com/trustedsec/COFFLoader/blob/main/beacon_generate.py):

```
Beacon Argument Generator
Beacon>addString C:\Users\wirt
Beacon>generate
b'120000000e000000433a5c55736572735c7769727400'
```

Then just provide your serialized arguments when linking the BOF:

```
$ cpl link ./bof2pico.spec /path/to/somebof.x64.o ./somebof.x64.pico '$BOF_ARGS=120000000e000000433a5c55736572735c7769727400'
```

