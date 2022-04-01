# Makefile for Proxy Lab 

CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread

PROXY = src/proxy.c
SIO = src/safe_io/sio.c
SOCKET_INTERFACE = src/socket_interface/interface.c
PROXY_SERVE = src/proxy_serve/serve.c
PROXY_CACHE = src/proxy_cache/cache.c
HEADERS = $(wildcard src/**/*.h)

all: proxy

sio.o: $(SIO) $(HEADERS)
	$(CC) $(CFLAGS) -c $(SIO)

interface.o: $(SOCKET_INTERFACE) $(HEADERS)
	$(CC) $(CFLAGS) -c $(SOCKET_INTERFACE)

serve.o: $(PROXY_SERVE) $(HEADERS)
	$(CC) $(CFLAGS) -c $(PROXY_SERVE)

cache.o: $(PROXY_CACHE) $(HEADERS)
	$(CC) $(CFLAGS) -c $(PROXY_CACHE)

proxy.o: $(PROXY)
	$(CC) $(CFLAGS) -c $(PROXY)

proxy: serve.o sio.o interface.o cache.o proxy.o
	$(CC) $(CFLAGS) serve.o sio.o interface.o cache.o proxy.o -o proxy $(LDFLAGS)

clean:
	rm -f *~ *.o proxy
