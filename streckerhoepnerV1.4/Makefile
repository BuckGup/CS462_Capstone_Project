CC = g++

all: recvfile sendfile

recvfile: src/receiver.cpp
	$(CC) -std=c++11 -pthread src/receiver.cpp -o recvfile

sendfile: src/sender.cpp
	$(CC) -std=c++11 -pthread src/sender.cpp -o sendfile

clean: recvfile sendfile
	rm -f recvfile sendfile
