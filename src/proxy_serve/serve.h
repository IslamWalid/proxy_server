#ifndef _SERVER_H_
#define _SERVER_H_

#include <sys/types.h>

#define MAX_LINE    8192        /* 8KB line buffer */
#define MAX_BUF     1048576     /* 1MB buffer size */
#define PORT_LEN    10          /* 10B port length */
#define VERSION_LEN 10          /* 10B http version length */
#define METHOD_LEN  10          /* 10B method length */

typedef struct request {
    char *rq_method;
    char *rq_hostname;
    char *rq_port;
    char *rq_path;
    char *rq_hdrs;
} Request;

typedef struct response {
    char *rs_line;
    char *rs_hdrs;
    void *rs_content;
    size_t rs_content_length;
} Response;

int
parse_request(int clientfd, Request *client_request);

int
forward_client_request(const Request *client_request, Response *server_response);

int
forward_server_response(int clientfd, const Response *server_response);

#endif
