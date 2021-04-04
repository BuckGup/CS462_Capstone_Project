# Sliding Window Algorithm
![Semver](http://img.shields.io/SemVer/0.1.0.png)  

Transfers data over UDP protocol using Sliding Window Algorithm implemented in C++.

## Brief Description
The main functionality of this project is transfering data over UDP protocol using Sliding Window Algorithm. This project is written in C++.  

### Project Structure
This project consists of 2 main programs and in total of 5 files that contain data structure definitions as well as helper functions for the main program. All source codes are located in `src/` directory. The `data/` directory contains sample data which are to be transferred by the receiver to the sender.  

### Directory Tree
Below is the directory tree for this project.  
```
udp-sliding-window/
├── data/
│   └── test.txt
├── src/
│   ├── frame.h
│   ├── frame.cpp
│   ├── utils.h
│   ├── sender.cpp
│   └── receiver.cpp
└── Makefile
```

## Getting Started
This project is developed and tested on machines running Debian 9 and Ubuntu 16.  

### Installation
For building this project, a `Makefile` file is included for convenience. In the project root directory, type in the following command in terminal.  
```
$ make
```
This will create 2 executables, `sendfile` for sending files and `recvfile` for receiving files.  

### Usage
This project consists of 2 executables, `sendfile` that acts as the sender and `recvfile` that acts as the receiver.  
>**Note:** Make sure that the sender and receiver runs on the same network.

#### Send
The `sendfile` executable takes 5 extra arguments. To run `sendfile`, in terminal type in the following command.  
```
$ ./sendfile <filename> <windowsize> <buffersize> <destination_ip_address> <destination_port>
```
Here is the explanation for each argument:
- `./sendfile` runs the `sendfile` executable.
- `<filename>` is the name of the file which is to be transported.
- `<windowsize>` is the size of the sender sliding window (SSW) in `n * 1024 bytes`.
- `<buffersize>` is size of the buffer in `n * 1024 bytes`.
- `<destination_ip_address>` is the IP address of the receiver.
- `<destination_port>` is the port where the receiver would receive the data.

Execution example:  
```
$ ./sendfile test.txt 5 100 127.0.0.1 9000
```
This means it sends `test.txt` to `localhost:9000` with SWS is `5 * 1024 bytes` and buffer size is `100 * 1024 bytes`.  

#### Receive
The `receiver` executable takes 4 extra arguments. To run `receiver`, in terminal type in the following command.  
```
$ ./recvfile <filename> <windowsize> <buffersize> <port>
```
Here is the explanation for each argument:
- `./recvfile` runs the `recvfile` executable.
- `<filename>` is the file name of the received data which are to be stored.
- `<windowsize>` is the size of the receiver sliding window (RSW) in `n * 1024 bytes`.
- `<buffersize>` is size of the buffer in `n * 1024 bytes`.
- `<port>` is the port to listen in order to receive the data.

Execution example:  
```
$ ./recvfile test.txt 5 100 9000
```
This means it receives data in port `9000` and stores the data in `test.txt` with SWS is `5 * 1024 bytes` and buffer size is `100 * 1024 bytes`.  

## How The Program Works
Sliding windows works on both sending side and receiving side.

On sending side, sender will read a file of given name then send it to receiver using sliding window protocol, the whole file will be sent divided into packets. After each packet sending process, sender will get feedback of ACK from receiver about sending status, whether the packet is well received or not. If it is not received well, then sender will send it back to receiver based on lost sequence number until right before LAF value.

On receiving side, each time receiving a packet, receiver will check on error status of packet. If there is an error, then receiver will send NAK to notify sender so that it can resend the packet. If there is no error, then receiver will send ACK of packet's next sequence number.

## Final Statements

### About This Project
This project is for IF3130 Computer Networks course (Fall 2018 term) first assignment, Computer Science Undergraduate Program, Institut Teknologi Bandung.

### Contributors
- Dionesius Agung Andika P (13516043)
- Dinda Yora I (13516067)
- Maharani Devira (13516142)
