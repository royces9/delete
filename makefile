CC = gcc

delete: delete.c delete.h
	gcc -Wall -o delete delete.c -lm

usr: delete.c delete.h
	gcc -o /usr/local/bin/del delete.c
