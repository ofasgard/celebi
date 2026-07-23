CC=i686-w64-mingw32-gcc
CC_64=x86_64-w64-mingw32-gcc
CFLAGS=-O1 -fno-jump-tables -shared -Wall -Wno-pointer-arith

bin:
	mkdir -p bin

libtcg:
	cd lib/libtcg/ && make all
	
x86: bin libtcg
	$(CC) -DWIN_X86 $(CFLAGS) -c src/bofapi.c -o bin/bofapi.x86.o
	$(CC) -DWIN_X86 $(CFLAGS) -c src/loader.c -o bin/loader.x86.o
	
x64: bin libtcg
	$(CC_64) -DWIN_X64 $(CFLAGS) -c src/bofapi.c -o bin/bofapi.x64.o
	$(CC_64) -DWIN_X64 $(CFLAGS) -c src/loader.c -o bin/loader.x64.o

all: x86 x64

clean:
	rm -rf bin
