# Sliding Window Algorithm
######Made by: Sam Strecker and Davis Hoepner

## Brief Description
Program that transfers data over UDP protocol using Sliding Window Algorithm implemented in C++.

## Getting Started
This project is developed and tested on the UWEC Phoenix machines running Redhat.

## Installation
For building this project, a `Makefile` file is included for convenience. In the project root directory, type in the following command in terminal.  
```
$ make
```
This will create 2 executables, `sendfile` for sending files and `recvfile` for receiving files.  

### Usage
This project consists of 2 executables, `sendfile` that acts as the sender and `recvfile` that acts as the receiver.  

## Send
The sendfile executable takes user input from the text-based UI. To run sendfile in the terminal type `./sendfile`

## Receive 
The sendfile executable takes argument flags in the terminal. To run sendfile in the terminal type `./recvfile <filename> <windowsize> <buffersize> <port>`

```
Explanation for each argument:
- `./recvfile` runs the `recvfile` executable.
- `<filename>` is the name of the new file created from the received data.
- `<windowsize>` is the size of the sliding window.
- `<buffersize>` is size of the buffer.
- `<port>` is the port to listen on.

Execution example:  

$ ./recvfile NewFile 10 1024 9191
```
This means it receives data on port `9191`, stores the data in `NewFile`, with a Sliding Windows size of `10`, and a
  buffer size is of `1024`.

#### How The Program Works
Please read the `TechnicalDocument.pdf` contained in this directory.