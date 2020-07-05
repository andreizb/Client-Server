# Makefile

CC = gcc
CFLAGS = -Wall -g

IP_SERVER = 127.0.0.1
PORT = 7777
ID = 1

all: server subscriber

# Compileaza server.c
server: server.o queue.o
	$(CC) $(CFLAGS) $^ -o $@ -lm

# Compileaza subscriber.c
subscriber: subscriber.c

queue.o: queue.c

.PHONY: clean run_server run_subscriber

# Ruleaza serverul
run_server:
	./server ${PORT}

# Ruleaza subscriberul
run_subscriber:
	./subscriber ${ID} ${IP_SERVER} ${PORT}

clean:
	rm -f server subscriber *.o
