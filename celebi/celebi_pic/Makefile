export CPL_PATH=/crystal-palace/dist/crystalpalace.jar

dirs:
	mkdir -p bin out
	
libtcg: lib/libtcg/
	cd lib/libtcg && make all
	
libwinhttp: lib/LibWinHttp/
	cd lib/LibWinHttp && make all

objects:
	x86_64-w64-mingw32-gcc $(CFLAGS) -DWIN_X64 -shared -masm=intel -Wall -Wno-pointer-arith -fno-toplevel-reorder -c src/main.c -o bin/main.o
	x86_64-w64-mingw32-gcc $(CFLAGS) -DWIN_X64 -shared -masm=intel -Wall -Wno-pointer-arith -fno-toplevel-reorder -c src/vault.c -o bin/vault.o
	x86_64-w64-mingw32-gcc $(CFLAGS) -DWIN_X64 -shared -masm=intel -Wall -Wno-pointer-arith -fno-toplevel-reorder -c src/message.c -o bin/message.o
	x86_64-w64-mingw32-gcc $(CFLAGS) -DWIN_X64 -shared -masm=intel -Wall -Wno-pointer-arith -fno-toplevel-reorder -c src/params.c -o bin/params.o
	x86_64-w64-mingw32-gcc $(CFLAGS) -DWIN_X64 -shared -masm=intel -Wall -Wno-pointer-arith -fno-toplevel-reorder -c src/pico.c -o bin/pico.o
	x86_64-w64-mingw32-gcc $(CFLAGS) -DWIN_X64 -shared -masm=intel -Wall -Wno-pointer-arith -fno-toplevel-reorder -c src/util.c -o bin/util.o

pico:
	x86_64-w64-mingw32-gcc $(CFLAGS) -DWIN_X64 -shared -masm=intel -Wall -Wno-pointer-arith -c src/pico/checkin.c -o bin/pico_checkin.o
	x86_64-w64-mingw32-gcc $(CFLAGS) -DWIN_X64 -shared -masm=intel -Wall -Wno-pointer-arith -c src/pico/whoami.c -o bin/pico_whoami.o
	
pic: dirs libtcg libwinhttp objects pico linker.spec
	java -Dcrystalpalace.verbose=false -jar ${CPL_PATH} buildPic ./linker.spec x64 out/main.bin @config.spec
	
clean:
	rm -f bin/*.o
	rm -f out/*.bin
	cd lib/libtcg && make clean
	cd lib/LibWinHttp && make clean
