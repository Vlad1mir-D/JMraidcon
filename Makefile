CC = gcc
CFLAGS = -g -O2 -Wall -std=gnu99
SUBDIRS = src
all:
	$(CC) $(CFLAGS) src/*.c -o JMraidcon

clean:
#	-rm -f JMraidcon src/*.o
	-rm -f JMraidcon
 
