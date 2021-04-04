CC = g++

all: recvfile sendfile

recvfile: src/frame.h src/frame.cpp src/utils.h src/receiver.cpp
	$(CC) -std=c++11 -pthread src/frame.cpp src/receiver.cpp -o recvfile

sendfile: src/frame.h src/frame.cpp src/utils.h src/sender.cpp
	$(CC) -std=c++11 -pthread src/frame.cpp src/sender.cpp -o sendfile

clean: recvfile sendfile
	rm -f recvfile sendfile
