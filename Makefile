filewipe: filewipe.o
	gcc -g -D__USE_GNU -D__USE_POSIX2 -D__USE_XOPEN2K8 -o filewipe filewipe.c

#filewipe.o: filewipe.c
#	gcc -g -D__USE_GNU -D__USE_POSIX2 -D__USE_XOPEN2K8 -o filewipe.o filewipe.c
