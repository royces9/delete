CC = gcc

delete: delete.c delete.h
	gcc -Wall -o delete delete.c -lm

debug: delete.c delete.h
	gcc -Wall -g -D DEBUG -o delete delete.c -lm

usr: delete.c delete.h
	gcc -o /usr/local/bin/del delete.c
