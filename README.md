# Konami test

## Description
Example program for a job interview\
Written by Tim Thompson\
Started March 11, 2025  at 9am

##  Build
There are 3 build targets, default, debug, and clean
#### Normal build (for 'prod')
```make```

#### Debug buid
```make debug```

#### Remove all generated output
```make clean```

## Summary
The program is split into 2 parts, client and server.\
The server is multi-threaded and has some tunible settings in constants.h.  I used a library called yxml to do the heavy lifting on the parsing\
[yxml](https://github.com/JulStrat/yxml)\
blah

## Usage
Note that the BENCHMARKS constant needs to be off to see output from the client and server programs.\
Per requiements, both programs have settable IP/port, and defaults as specified\
After building, run all items from the root directory of the project

To run the server with defaults:\
```./build/server```

To run the client with defaults:\
```./build/client ./messages/xml-message.txt```\
There are several messages to use in the messages directory

### Options
Both server and client programs will accept optional parms for ip and port\
server:
```./build/server 127.0.0.1 5010```\
client:\
```./build/client 127.0.0.1 5010 ./messages/xml-message.txt```


## Utilities
### gen_xml.py
I had XML on the brain so I coded something in Python quick to generate some of the message files\
Taking time to lay the example schema out so that the program generated the pattern in the requirements document was fun


## Testing
The good path works, and takes a decent amount of load with the flood shell script.  The client can't hammer the server nearly hard enough, it keeps up even with one thread.\

#### Error case: Malformed XML
Blah
#### Hard case: Target XML tag not the first element
Blah

## Bugs
The client lacks threading, it can't hammer the server nearly hard enough. I think it spends too much time building/tearing down sockets

## Final Thoughts
yxml integration Makefile hell







