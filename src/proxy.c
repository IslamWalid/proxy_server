#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "proxy_serve/serve.h"
#include "socket_interface/interface.h"

#define safe_free(ptr) sfree((void **) &(ptr))

typedef struct sockaddr SA;

static void
client_serve(int client_fd);

static void
free_resources(Request *request, Response *response);

static void
sfree(void **ptr);

int 
main(int argc, char **argv)
{
    int connfd, listenfd;
    char hostname[MAX_LINE], port[PORT_LEN];
    socklen_t client_len;
    struct sockaddr_storage client_addr;

    signal(SIGPIPE, SIG_IGN);

    /* Check command-line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    listenfd = open_listenfd(argv[1]);

    while (1) {
        client_len = sizeof(client_addr);
        if ((connfd = accept(listenfd, (SA* ) &client_addr, &client_len)) < 0) {
            fprintf(stderr, "Connection to (%s, %s) failed\n", hostname, port);
            continue;
        }
        client_serve(connfd);
        close(connfd);
    }
}

static void
client_serve(int client_fd)
{
    Request client_request;
    Response server_response;

    /* Initialize client_request and server_response structs with NULL */
    memset(&client_request, 0, sizeof(client_request));
    memset(&server_response, 0, sizeof(server_response));
    
    if (
    /* Parse the HTTP request */
    (parse_request(client_fd, &client_request) < 0) || 
    /* Forward the client request to the server after parsing successfully */
    (forward_client_request(&client_request, &server_response) < 0) ||
    /* Forward the server response to the client after requesting successfully */
    (forward_server_response(client_fd, &server_response) < 0)) {
        /* Free the resources and return when failing */
        free_resources(&client_request, &server_response);
        return;
    }

    free_resources(&client_request, &server_response);
}

static void
free_resources(Request *request, Response *response)
{
    safe_free(request->rq_method);
    safe_free(request->rq_hostname);
    safe_free(request->rq_port);
    safe_free(request->rq_path);
    safe_free(request->rq_hdrs);

    safe_free(response->rs_line);
    safe_free(response->rs_hdrs);
    safe_free(response->rs_content);
}

static void
sfree(void **ptr)
{
    if (ptr != NULL && *ptr != NULL) {
        free(*ptr);
        *ptr = NULL;
    }
}
