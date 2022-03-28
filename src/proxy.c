#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "proxy_cache/cache.h"
#include "proxy_serve/serve.h"
#include "socket_interface/interface.h"

#define safe_free(ptr) sfree((void **) &(ptr))

typedef struct sockaddr SA;

typedef struct vargp {
    Cache *proxy_cache;
    int clientfd;
} Vargp;

static void *
client_serve(void *vargp);

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
    pthread_t tid;
    Cache proxy_cache;
    Vargp *vargp;
    
    signal(SIGPIPE, SIG_IGN);

    /* Check command-line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    listenfd = open_listenfd(argv[1]);
    cache_init(&proxy_cache);
    while (1) {
        client_len = sizeof(client_addr);
        if ((connfd = accept(listenfd, (SA* ) &client_addr, &client_len)) < 0) {
            fprintf(stderr, "Connection to (%s, %s) failed\n", hostname, port);
            continue;
        }
        vargp = malloc(sizeof(Vargp));
        vargp->clientfd = connfd;
        vargp->proxy_cache = &proxy_cache;
        pthread_create(&tid, NULL, client_serve, vargp);
    }
}

static void *
client_serve(void *vargp)
{
    int clientfd;
    Cache *proxy_cache;
    Request client_request;
    Response server_response;

    clientfd = ((Vargp *) vargp)->clientfd;
    proxy_cache = ((Vargp *) vargp)->proxy_cache;
    free(vargp);

    pthread_detach(pthread_self());
    /* Initialize client_request and server_response structs with NULL */
    memset(&client_request, 0, sizeof(client_request));
    memset(&server_response, 0, sizeof(server_response));
    
    /* Parse the HTTP request */
    if (!(parse_request(clientfd, &client_request) < 0)) {
        /* Forward the client request to the server after parsing successfully */
        if (!(forward_client_request(&client_request, proxy_cache,
                                     &server_response) < 0)) {
            /* Forward the server response to the client after requesting 
             * successfully */
            if (!(forward_server_response(clientfd, &server_response) < 0)) {
                /* Action taking on successful serving */
            }
        }
    }

    free_resources(&client_request, &server_response);
    close(clientfd);
    return NULL;
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
