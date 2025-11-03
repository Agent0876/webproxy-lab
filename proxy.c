#include "csapp.h"
#include <stdio.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

static void *worker(void *vargp);
static void handle_client(int connfd);
static int parse_request_line(const char *line, char *method, char *uri, char *version);
static void parse_uri(const char *uri, char *host, char *port, char *path);
static void build_request_headers(rio_t *cr, char *host, char *port,
                                  char *outbuf, size_t outcap, const char *path);

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  int listenfd = Open_listenfd(argv[1]);

  while (1)
  {
    struct sockaddr_storage clientaddr;
    socklen_t len = sizeof(clientaddr);
    int connfd = Accept(listenfd, (SA *)&clientaddr, &len);

    pthread_t tid;
    Pthread_create(&tid, NULL, worker, (void *)(long)connfd);
    Pthread_detach(tid);
  }
  return 0;
}

static void *worker(void *vargp)
{
  int connfd = (int)(long)vargp;
  handle_client(connfd);
  Close(connfd);
  return NULL;
}

static void handle_client(int connfd)
{
  
}