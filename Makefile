CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -g -O3 -pthread
LDLIBS=-lm
JAVAC=javac

EXECS=msf.out
OBJS=msf.o hash_table.o union_find.o linked_list.o abr.o queue.o

all: $(EXECS)

$(EXECS): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

Msf.class:
	$(JAVAC) Msf.java

clean:
	rm -f $(OBJS) $(EXECS) *.class
