# Makefile for Proxy Lab 

CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread

PROXY = proxy.c
SIO = $(wildcard safe_io/sio.c)
SOCKET_INTERFACE = $(wildcard socket_interface/interface.c)
PROXY_SERVE = $(wildcard proxy_serve/serve.c)
PROXY_CACHE = $(wildcard proxy_cache/cache.c)

all: proxy

sio.o: $(SIO)
	$(CC) $(CFLAGS) -c $(SIO)

interface.o: $(SOCKET_INTERFACE)
	$(CC) $(CFLAGS) -c $(SOCKET_INTERFACE)

serve.o: $(PROXY_SERVE)
	$(CC) $(CFLAGS) -c $(PROXY_SERVE)

cache.o: $(PROXY_CACHE)
	$(CC) $(CFLAGS) -c $(PROXY_CACHE)

proxy.o: $(PROXY)
	$(CC) $(CFLAGS) -c $(PROXY)

proxy: serve.o sio.o interface.o cache.o proxy.o
	$(CC) $(CFLAGS) serve.o sio.o interface.o cache.o proxy.o -o proxy $(LDFLAGS)

clean:
	rm -f *~ *.o proxy
