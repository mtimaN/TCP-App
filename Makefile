CXXFLAGS = -Wall -Wextra -Werror
# Server listening port
PORT = 12345

# Subscriber ID, default = 0
CLIENT_ID = "C1"

SERVER_IP = 127.0.0.1

OBJ = common.o parser.o

all: subscriber server

%.o: %.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS)

server: $(OBJ) server.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS)

subscriber: $(OBJ) subscriber.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS)

.PHONY: clean run_server run_subscriber

# Ruleaza serverul
run_server:
	./server ${PORT}

# Ruleaza subscriber
run_subscriber:
	./subscriber ${CLIENT_ID} ${SERVER_IP} ${PORT}

clean:
	rm -rf server subscriber *.o *.dSYM
