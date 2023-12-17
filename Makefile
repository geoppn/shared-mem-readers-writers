CC = gcc
CFLAGS = -Wall -g

all: reader writer main

reader: reader.c SharedMemory.c
	$(CC) $(CFLAGS) -o reader reader.c SharedMemory.c

writer: writer.c SharedMemory.c
	&(CC) $(CFLAGS) -o writer writer.c SharedMemory.c

main: main.c SharedMemory.c
	$(CC) $(CFLAGS) -o main main.c SharedMemory.c

clean:
	rm -f reader writer main