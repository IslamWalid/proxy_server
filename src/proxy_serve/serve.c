#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "serve.h"
#include "../safe_io/sio.h"
#include "../socket_interface/interface.h"

#define USED_HDRS_SIZE 130

static int
parse_request_line(Sio *sio, char *method, char *url);

static void
parse_url(const char *url, char *hostname, char *port, char *path);

static int
parse_request_hdrs(Sio *sio, char *request_hdrs, char *hostname);

static void
build_request_line(const Request *request, char *request_line);

static void
build_request_hdrs(const Request *request, char *request_hdrs);

static int
parse_response(int connfd, Response *response);

static ssize_t
parse_response_hdrs(Sio *sio, char *response_hdrs);

static void
client_error(int clientfd, char *cause, char *errnum,
             char *short_msg, char *long_msg);

static void
strtolwr(char *str);

static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:98.0) Gecko/20100101 Firefox/98.0\r\n";
static const char *proxy_conn_hdr = "Proxy-Connection: close\r\n";
static const char *conn_hdr = "Connection: close\r\n";

int
parse_request(int clientfd, Request *client_request)
{
    Sio sio;
    char method[METHOD_LEN], url[MAX_LINE],
    hostname[MAX_LINE], port[PORT_LEN], path[MAX_LINE], request_hdrs[MAX_BUF];

    /* Initialize safe read buffer associated with the clinetfd */
    sio_initbuf(&sio, clientfd);

    if (parse_request_line(&sio, method, url) < 0)
        return -1;

    if (parse_request_hdrs(&sio, request_hdrs, hostname) < 0)
        return -1;

    parse_url(url, hostname, port, path);

    /* Build the client request struct */
    client_request->rq_method = strdup(method);
    client_request->rq_hostname = strdup(hostname);
    client_request->rq_port = strdup(port);
    client_request->rq_path = strdup(path);
    client_request->rq_hdrs = strdup(request_hdrs);

    return 0;
}

int
forward_client_request(const Request *client_request, Response *server_response)
{
    int connfd;
    char request_line[MAX_LINE], request_hdrs[MAX_BUF];

    /* Establish TCP connection with the server */
    connfd = open_clientfd(client_request->rq_hostname, client_request->rq_port);
    if (connfd < 0)
        return -1;

    /* Build the HTTP request line to be sent to the server */
    build_request_line(client_request, request_line);
    /* Build the HTTP request headers to be sent to the server */
    build_request_hdrs(client_request, request_hdrs);

    /* Send the http request to the server */
    if (sio_writen(connfd, request_line, strlen(request_line)) < 0)
        return -1;
    if (sio_writen(connfd, request_hdrs, strlen(request_hdrs)) < 0)
        return -1;

    /* Parse the server's response */
    if (parse_response(connfd, server_response) < 0)
        return -1;

    return 0;
}

int
forward_server_response(int clientfd, const Response *server_response)
{
    if (sio_writen(clientfd, server_response->rs_line,
                   strlen(server_response->rs_line)) < 0)
        return -1;

    if (sio_writen(clientfd, server_response->rs_hdrs,
                   strlen(server_response->rs_hdrs)) < 0)
        return -1;

    if (sio_writen(clientfd, server_response->rs_content,
                   server_response->rs_content_length) < 0)
        return -1;

    return 0;
}

static int
parse_request_line(Sio *sio, char *method, char *url)
{
    char version[VERSION_LEN], request_line[MAX_LINE];

    if (sio_read_line(sio, request_line, MAX_LINE) < 0)
        return -1;

    if (sscanf(request_line, "%s %s %s", method, url, version) != 3) {
        client_error(sio->sio_fd, request_line, "400", "Bad request",
                     "Request could not be understood by the proxy server");
        return -1;
    }

    if (strcmp(method, "GET")) {
        client_error(sio->sio_fd, method, "501", "Not implemented",
                     "Server does not support the request method");
        return -1;
    }

    if (strcmp(version, "HTTP/1.0") && strcmp(version, "HTTP/1.1")) {
        client_error(sio->sio_fd, method, "505", "HTTP version not supported",
                     "Server does not support version in request");
        return -1;
    }

    return 0;
}

static void
parse_url(const char *url, char *hostname, char *port, char *path)
{
    char *url_copy, *url_token;

    url_copy = strdup(url);
    url_token = url_copy;

    /* Skip the "http://" part of the url */
    url_token = strstr(url, "://");
    url_token += 3;

    if (strstr(url_token, ":")) {     /* Port is contained in the url */
        /* Url token points to the hostname */
        url_token = strtok(url_token, ":");
        strcpy(hostname, url_token);
        
        /* Move url token to the port and path token */
        url_token = strtok(NULL, ":");

        if (strstr(url_token, "/")) {   /* Path is contained in the url */
            /* Parse the port from url_token */
            url_token = strtok(url_token, "/");  
            strcpy(port, url_token);

            /* Parse the path from url_token */
            url_token = strtok(NULL, "/");
            strcpy(path, url_token);
        } else {                        /* Path is not contained in the url */
            strcpy(port, url_token);
            strcpy(path, "/");  /* Set path to default */
        }

    } else {    /* Port is not contained in the url */
        strcpy(port, "80");     /* Set port to default */

        url_token = strtok(url_token, "/");
        strcpy(hostname, url_token);

        url_token = strtok(NULL, "/");

        if (url_token)          /* Path is contained in the url */
            strcpy(path, url_token);
        else                    /* Path is not contained in the url */
         strcpy(path, "/");     /* Path is set to default */
    }
    free(url_copy);
}

static int
parse_request_hdrs(Sio *sio, char *request_hdrs, char *hostname)
{
    ssize_t nread = MAX_BUF - USED_HDRS_SIZE;
    char hdr_linebuf[MAX_LINE], hostnamebuf[MAX_LINE];

    /* Initialize request_hdrs to be ready for appending (concatination) */
    request_hdrs[0] = '\0';
    do {
        if (sio_read_line(sio, hdr_linebuf, MAX_LINE) < 0)
            return -1;
        if (sscanf(hdr_linebuf, "Host: %s", hostnamebuf) == 1)
            strcpy(hostname, hostnamebuf);
        strncat(request_hdrs, hdr_linebuf, nread);
        nread -= strlen(hdr_linebuf);
    } while (strcmp(hdr_linebuf, "\r\n") && nread > 0);

    return 0;
}

static void
build_request_line(const Request *request, char *request_line)
{
    request_line[0] = '\0';
    strcat(request_line, request->rq_method);
    strcat(request_line, " ");

    strcat(request_line, request->rq_path);
    strcat(request_line, " ");

    strcat(request_line, "HTTP/1.0\r\n");
}

static void
build_request_hdrs(const Request *request, char *request_hdrs)
{
    request_hdrs[0] = '\0';
    strcat(request_hdrs, user_agent_hdr);
    strcat(request_hdrs, conn_hdr);
    strcat(request_hdrs, proxy_conn_hdr);
    strcat(request_hdrs, request->rq_hdrs);
}

static int
parse_response(int connfd, Response *response)
{
    Sio sio;
    ssize_t content_len;
    char response_line[MAX_LINE], response_hdrs[MAX_BUF];

    sio_initbuf(&sio, connfd);

    /* Parse response line */ 
    if (sio_read_line(&sio, response_line, MAX_LINE) < 0)
        return -1;
    /* Parse response headrs */
    if ((content_len = parse_response_hdrs(&sio, response_hdrs)) < 0)
        return -1;
    /* Parse content */
    response->rs_content = malloc(content_len);
    if (sio_readn(&sio, response->rs_content, content_len) < 0)
        return -1;

    /* Build the response struct */
    response->rs_content_length = content_len;
    response->rs_line = strdup(response_line);
    response->rs_hdrs = strdup(response_hdrs);

    return 0;
}

static ssize_t
parse_response_hdrs(Sio *sio, char *response_hdrs)
{
    ssize_t content_len = -1;
    char hdr_linebuf[MAX_LINE];

    /* Initialize request_hdrs to be ready for appending (concatination) */
    response_hdrs[0] = '\0';
    do {
        if (sio_read_line(sio, hdr_linebuf, MAX_LINE) < 0)
            return -1;

        strcat(response_hdrs, hdr_linebuf);
        strtolwr(hdr_linebuf);
        sscanf(hdr_linebuf, "content-length: %zu", &content_len);
    } while (strcmp(hdr_linebuf, "\r\n"));

    return content_len;
}

static void
client_error(int clientfd, char *cause, char *errnum,
             char *short_msg, char *long_msg)
{
    char linebuf[MAX_LINE], body[MAX_BUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Proxy Error</title>");
    strcat(body, "<body bgcolor=""ffffff"">\r\n");
    sprintf(linebuf, "%s: %s\r\n", errnum, short_msg);
    strcat(body, linebuf);
    sprintf(linebuf, "%s: %s\r\n", long_msg, cause);
    strcat(body, linebuf);

    /* Send the HTTP response */
    sprintf(linebuf, "HTTP/1.0 %s %s\r\n", errnum, short_msg);
    if (sio_writen(clientfd, linebuf, strlen(linebuf)) < 0)
        return;
    sprintf(linebuf, "Content-Type: text/html\r\n");
    if (sio_writen(clientfd, linebuf, strlen(linebuf)) < 0)
        return;
    sprintf(linebuf, "Content-Length: %zu\r\n\r\n", strlen(body));
    if (sio_writen(clientfd, linebuf, strlen(linebuf)) < 0)
        return;
    if (sio_writen(clientfd, body, strlen(body)) < 0)
        return;
}

static void
strtolwr(char *str)
{
    for (int i = 0; str[i] != '\0'; i++) {
        if (isalpha(str[i]))
            str[i] = tolower(str[i]);
    }
}
