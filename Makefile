all:
	gcc -Wall -Wpedantic -o a \
	*.c \
	fileio/*.c \
	log/*.c
	
