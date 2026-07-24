x86:
	# Load the BOF and make it into a PICO.
	push $OBJECT
	make object +optimize
	
	# Set _go() as the new entrypoint and merge in BOF APIs and hooks.
	entry "_go"
	
	load "bin/bofapi.x86.o"
		merge
		
	load "bin/loader.x86.o"
		merge
	
	# Hook BOF APIs.
	attach "$BeaconDataExtract"    "_BeaconDataExtract"
	attach "$BeaconDataLength"     "_BeaconDataLength"
	attach "$BeaconDataParse"      "_BeaconDataParse"
	attach "$BeaconDataPtr"        "_BeaconDataPtr"
	attach "$BeaconDataInt"        "_BeaconDataInt"
	attach "$BeaconDataShort"      "_BeaconDataShort"

	attach "$BeaconFormatAlloc"    "_BeaconFormatAlloc"
	attach "$BeaconFormatReset"    "_BeaconFormatReset"
	attach "$BeaconFormatAppend"   "_BeaconFormatAppend"
	attach "$BeaconFormatPrintf"   "_BeaconFormatPrintf"
	attach "$BeaconFormatToString" "_BeaconFormatToString"
	attach "$BeaconFormatFree"     "_BeaconFormatFree"
	attach "$BeaconFormatInt"      "_BeaconFormatInt"
	attach "$BeaconPrintf"         "_BeaconPrintf"
	attach "$BeaconOutput"         "_BeaconOutput"

	# No arguments.
	pack $BOF_ARGS "i" 0x0
	push $BOF_ARGS
	link "bargs"
	
	# Export the PICO.
	export

x64:
	# Load the BOF and make it into a PICO.
	push $OBJECT
	make object +optimize
	
	# Set _go() as the new entrypoint and merge in BOF APIs and hooks.
	entry "_go"
	
	load "bin/bofapi.x64.o"
		merge
		
	load "bin/loader.x64.o"
		merge
	
	# Hook BOF APIs.
	attach "$BeaconDataExtract"    "BeaconDataExtract"
	attach "$BeaconDataLength"     "BeaconDataLength"
	attach "$BeaconDataParse"      "BeaconDataParse"
	attach "$BeaconDataPtr"        "BeaconDataPtr"
	attach "$BeaconDataInt"        "BeaconDataInt"
	attach "$BeaconDataShort"      "BeaconDataShort"

	attach "$BeaconFormatAlloc"    "BeaconFormatAlloc"
	attach "$BeaconFormatReset"    "BeaconFormatReset"
	attach "$BeaconFormatAppend"   "BeaconFormatAppend"
	attach "$BeaconFormatPrintf"   "BeaconFormatPrintf"
	attach "$BeaconFormatToString" "BeaconFormatToString"
	attach "$BeaconFormatFree"     "BeaconFormatFree"
	attach "$BeaconFormatInt"      "BeaconFormatInt"
	attach "$BeaconPrintf"         "BeaconPrintf"
	attach "$BeaconOutput"         "BeaconOutput"
	
	# No arguments.
	pack $BOF_ARGS "i" 0x0
	push $BOF_ARGS
	link "bargs"
	
	# Export the PICO.
	export
