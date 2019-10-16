# Author:   Colby Ackerman
# Class:    CS4760 Operating Systems
# Assign:   #4
# Date:     10/22/19
#-----------------------------------------------------

CC = gcc
CFLAGS = -I. -g
.SUFFIXES: .c .o

all: oss usrPs

oss: oss.o
	$(CC) $(CFLAGS) -o $@ oss.o -lpthread

usrPs: userPs.o
	$(CC) $(CFLAGS) -o $@ userPs.o -lpthread

.c.o:
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm -f *.o oss usrPs *.txt