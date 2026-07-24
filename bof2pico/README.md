# bof2pico

Helper utility for converting a compiled BOF into a PICO that celebi is able to execute. Adapted from [Simple BOF Runner](https://tradecraftgarden.org/simplebof.html) by Raphael Mudge, and reproduces the `bofapi.c` file in its entirety.

Usage:

```sh
$ make all
$ cpl link ./bof2pico.spec /path/to/somebof.x64.o ./somebof.x64.pico
```

## Passing Arguments

By default, no arguments will be passed to the BOF. If you want to pass arguments, modify `bof2pico.spec` and change this:

```
# No arguments.
pack $BOF_ARGS "i" 0x0
push $BOF_ARGS
link "bargs"
```

To something like this:

```
# Pack hardcoded arguments in BOF argument format.
setg "%ARG1" "wirt"
setg "%ARG2" "WSearch"
pack $BOF_ARGS "iziz" 5 %ARG1 8 %ARG2

# Link hardcoded arguments to the PICO.
push $BOF_ARGS
preplen
link "bargs"
```

The BOF argument format is fairly simple, but you'll need to know what arguments your BOF is expecting. I recommend using [COFFLoader](https://github.com/trustedsec/COFFLoader/blob/main/beacon_generate.py) as a reference.
