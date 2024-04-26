CFLAGS = -Wall -g -Werror -Wno-error=unused-variable -std=c99

# Portul pe care asculta serverul
PORT = 12345

# Adresa IP a serverului
IP_SERVER = 127.0.0.1

all: server subscriber

common.o: common.cpp

server: server.cpp common.o

subscriber: subscriber.cpp common.o

.PHONY: clean run_server run_subscriber

# Ruleaza serverul
run_server:
	./server ${IP_SERVER} ${PORT}

# Ruleaza subscriber
run_subscriber:
	./subscriber ${IP_SERVER} ${PORT}

clean:
	rm -rf server client *.o *.dSYM
