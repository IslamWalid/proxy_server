# Proxy Server
A **Web proxy** is a program that acts as a **middleman** between a Web browser and an end server. Instead of
contacting the end server directly to get a Web page, the browser contacts the proxy, which forwards the
request on to the end server. When the end server replies to the proxy, the proxy sends the reply on to the
browser.

## Overview
- It is the 7th and the last lab of [15-213: Introduction to Computer Systems](https://www.cs.cmu.edu/afs/cs.cmu.edu/academic/class/15213-f15/www/index.html).
- The coding style and convention is inspired from [suckless coding style](https://suckless.org/coding_style/).
- It is a `multi-threaded` program to be capable of handling multiple connections at the same time.
- It can cache web objects by storing copies of the responses it gets from the server to use it to respond immediately to any future connections to the same server. Its cache follows LRU eviction policy.

**NOTE:** All project specifications are described in [ProxyLab write-up](https://github.com/IslamWalid/proxy_server/blob/master/proxylab.pdf).

## How this proxy works?
- The main thread accepts connect requests from clients.
- For each client, the main thread creates a thread and pass the file descriptor of the client to be serverd.
- All created threads dipatches itself after freeing the resources it allocated for serving the client.

## Program modules and implementation details
**[`proxy.c`](https://github.com/IslamWalid/proxy_server/blob/master/src/proxy.c) contains the `main` function, as well as `serve` function as described below:**

- The **`main`** function does the following steps:

    1) ***Open*** a listen port to recieve connection requests from clients.
    2) ***Create*** and initialize a cache to be shared between all worker threads.
    3) ***Accept*** and establish connection with each client.
    4) ***Create*** a thread and pass the shared cache and the client file descriptor to it.

- The **`serve`** function does the following steps:

    1) ***Read and parse*** the request the client sent.

    2) ***Forward*** the read request to the server.

    3) ***Get*** the response from the server, then forward it to the client.

    4) ***Free*** the allocated resources and close the connection with the client.

**[`proxy_server`](https://github.com/IslamWalid/proxy_server/tree/master/src/proxy_serve) provides the `serve` function described before with the following three functions:**

- It provides **`serve`** function with the following three functions:
    - **`parse_request`**:

        1) ***Parse*** the request line from the HTTP request.
        2) ***Extract*** the method, uri and HTTP version from the request line.
        3) ***Parse*** the request headers.

    - **`forward_client_request`:**

        1) ***Build*** the request line that will be sent to the server.
        2) ***Build*** the request headers.
        3) ***Search*** in the cache for an element that matches the built request line and headers, then return it.
        4) ***Connect*** to the server and caches it, then returns it.

    - **`forward_server_response`:**
        
        1) ***Send*** the response back to the client.

**[`safe_io`](https://github.com/IslamWalid/proxy_server/tree/master/src/safe_io):**
- It provide safe and re-entrant functions to read and write data to connnection sockets.

**[`socket_interface`](https://github.com/IslamWalid/proxy_server/tree/master/src/socket_interface):**
- It provides an abstraction layer above the [standard socket interface library](https://www.gnu.org/software/libc/manual/html_node/Sockets.html) to create TCP sockets.
- It supports TCP server or client sockets.
- It contains two functions:

    - `open_client`: establish a client socket connected to a server with the given hostname and port.
    - `open_server`: creates a listen socket associated with the given port to be ready to recieve TCP connection requests.

## Requirements
- `linux`
- `git`
- `gcc`
- `make`

## How to test and use it?
**1) Download, compile and run the source code as following:**
```
git clone https://github.com/IslamWalid/proxy_server.git
```
```
cd proxy_server
```
```
make
```
```
./proxy <port>
```

**2) Connect to the proxy and send an HTTP request to the server using:**
- **telnet:**
    ```
    telnet localhost <port>
    GET http://www.example.com HTTP/1.0
    ```
- **browser:**
    - Change your browser settings to connect to the proxy.
    - Connect to an HTTP website, for ex: `http//www.example.com`

**NOTE:** Connect to http websites (ex: websites mentioned [here](https://github.com/IslamWalid/proxy_server/blob/master/http_web_sites.txt)), or use [tiny_web_server](https://github.com/IslamWalid/tiny_web_server) which I developed to be used in testing this proxy.
