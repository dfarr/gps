# Makefile for Assignment 3

# --- macros
CC=gcc
CFLAGS=-Wall -std=c99 -pedantic -g
OBJECTS=gputil.o gpstool.o
LIBS=-L -lefence

# --- macros for the shared part
SFLAGS=-Wall -I/usr/include/python2.7 -fPIC -std=c99 -pedantic -g

# --- targets
all: gpstool

gpstool: $(OBJECTS) 
	$(CC) -o gpstool $(OBJECTS) $(LIBS)
	$(CC) $(SFLAGS) -c shared.c 
	$(CC) -shared shared.o -o Gps.so
        
gpstool.o: gpstool.c gputil.o
	$(CC) $(CFLAGS) -c gpstool.c
       
gputil.o: gputil.h gpstool.h gputil.c
	$(CC) $(CFLAGS) -c gputil.c 

# --- remove binary and executable files
clean:
	rm -f gpstool $(OBJECTS) shared.o Gps.so