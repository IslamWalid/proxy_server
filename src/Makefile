# Makefile for Proxy Lab 

CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread

PROXY = proxy.c
SIO = $(wildcard safe_io/sio.c)
SOCKET_INTERFACE = $(wildcard socket_interface/interface.c)
PROXY_SERVE = $(wildcard proxy_serve/serve.c)

all: proxy

sio.o: $(SIO)
	$(CC) $(CFLAGS) -c $(SIO)

interface.o: $(SOCKET_INTERFACE)
	$(CC) $(CFLAGS) -c $(SOCKET_INTERFACE)

serve.o: $(PROXY_SERVE)
	$(CC) $(CFLAGS) -c $(PROXY_SERVE)

proxy.o: $(PROXY)
	$(CC) $(CFLAGS) -c $(PROXY)

proxy: serve.o sio.o interface.o proxy.o
	$(CC) $(CFLAGS) serve.o sio.o interface.o proxy.o -o proxy $(LDFLAGS)

clean:
	rm -f *~ *.o proxy
