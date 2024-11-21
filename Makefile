all:
	gcc -Wall -Wextra -Wpedantic -Wconversion -Wno-unused-parameter -o a \
	*.c \
	allocators/*.c \
	fileio/*.c \
	log/*.c

scratch:
	gcc -o scratch scratch.c
	
clean:
	rm logs/* a scratch
