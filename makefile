# Author:   Colby Ackerman
# Class:    CS4760 Operating Systems
# Assign:   #4
# Date:     10/22/19
#-----------------------------------------------------

CC = gcc
CFLAGS = -I. -g
OBJECTS = sharedMem.o
.SUFFIXES: .c .o

all: oss usrPs

oss: oss.o $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ oss.o $(OBJECTS) -lpthread

usrPs: userPs.o $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ userPs.o $(OBJECTS) -lpthread

.c.o:
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm -f *.o oss usrPs *.txt