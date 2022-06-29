CFLAGS = -Wall -g

PORT = 12343

IP_SERVER = 127.0.0.1

all: server subscriber

server: server.cpp

subscriber: subscriber.cpp

.PHONY: clean run_server run_client

clean:
	rm -f server subscriber
