# Server listening port
PORT = 8888

# Subscriber ID, default = 0
CLIENT_ID = ""

SERVER_IP = 127.0.0.1

all: server subscriber

common.o: common.cpp

server: server.cpp common.o

subscriber: subscriber.cpp common.o

.PHONY: clean run_server run_subscriber

# Ruleaza serverul
run_server:
	./server ${PORT}

# Ruleaza subscriber
run_subscriber:
	./subscriber ${CLIENT_ID} ${SERVER_IP} ${PORT}

clean:
	rm -rf server subscriber *.o *.dSYM
