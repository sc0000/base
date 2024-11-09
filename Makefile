all:
	gcc -Wall -Wpedantic -o a \
	*.c \
	allocators/*.c \
	fileio/*.c \
	log/*.c
	
