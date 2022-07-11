all: filewipe filewipe.exe

filewipe: filewipe.c
	gcc -g -D__USE_GNU -D__USE_POSIX2 -D__USE_XOPEN2K8 -o filewipe filewipe.c

#filewipe.o: filewipe.c
#	gcc -g -D__USE_GNU -D__USE_POSIX2 -D__USE_XOPEN2K8 -o filewipe.o filewipe.c

filewipe.exe: filewipe.c
	x86_64-w64-mingw32-gcc -DWINDOWS -mconsole -o filewipe.exe filewipe.c
