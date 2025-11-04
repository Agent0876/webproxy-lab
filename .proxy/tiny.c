/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

// 할 작업 한글로 주석 달기

int main(int argc, char **argv)
{
  // argc 명령행 인자 개수, argv 명령행 인자 배열
  // argv는 main 함수에 전달된 인자들을 문자열 배열로 저장
  // ./ tiny <port> 형태로 실행 되므로 argv[0]은 프로그램 이름, argv[1]은 포트 번호
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  // 명령행 인자 개수 확인
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  // 서버 소켓 열기
  listenfd = Open_listenfd(argv[1]);

  // 무한 루프를 돌며 클라이언트 연결 처리
  while (1)
  {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen);
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);
    Close(connfd);
  }
}

void doit(int fd)
{
  // fd: 클라이언트와의 연결을 나타내는 파일 디스크립터
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);

  if (strcasecmp(method, "GET"))
  {
    clienterror(fd, method, "501", "Not Implemented", "Tiny does not implement this method");
    return;
  }

  read_requesthdrs(&rio);

  is_static = parse_uri(uri, filename, cgiargs);

  if (stat(filename, &sbuf) < 0)
  {
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
  }

  if (is_static)
  {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
    serve_static(fd, filename, sbuf.st_size);
  }
  else
  {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs);
  }
}

void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  printf("%s", buf);
  while (strcmp(buf, "\r\n"))
  {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  if (!strstr(uri, "cgi-bin"))
  {
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    if (uri[strlen(uri) - 1] == '/')
      strcat(filename, "home.html");
    return 1;
  }
  else
  {
    ptr = strchr(uri, '?');
    if (ptr)
    {
      strcpy(cgiargs, ptr + 1);
      *ptr = '\0';
    }
    else
      strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}

void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];
  int n = 0;

  get_filetype(filename, filetype);

  n = snprintf(buf, sizeof(buf), "HTTP/1.0 200 OK\r\n");
  n += snprintf(buf + n, sizeof(buf) - n, "Server: Tiny Web Server\r\n");
  n += snprintf(buf + n, sizeof(buf) - n, "Connection: close\r\n");
  n += snprintf(buf + n, sizeof(buf) - n, "Content-length: %d\r\n", filesize);
  n += snprintf(buf + n, sizeof(buf) - n, "Content-type: %s\r\n\r\n", filetype);

  if (n < 0 || n >= (int)sizeof(buf))
  {
    char fallback[] =
        "HTTP/1.0 500 Internal Server Error\r\n"
        "Connection: close\r\n\r\n";
    Rio_writen(fd, (void *)fallback, strlen(fallback));
    return;
  }

  Rio_writen(fd, buf, n);

  srcfd = Open(filename, O_RDONLY, 0);
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  Close(srcfd);
  Rio_writen(fd, srcp, filesize);
  Munmap(srcp, filesize);
}

void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else
    strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] = {NULL};

  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if (Fork() == 0)
  {
    setenv("QUERY_STRING", cgiargs, 1);
    Dup2(fd, STDOUT_FILENO);
    Execve(filename, emptylist, environ);
  }
  Wait(NULL);
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];
  int n = 0;

  n = snprintf(body, sizeof(body), "<html><title>Tiny Error</title>");
  n += snprintf(body + n, sizeof(body) - n, "<body bgcolor=\"ffffff\">\r\n");
  n += snprintf(body + n, sizeof(body) - n, "%s: %s\r\n", errnum, shortmsg);
  n += snprintf(body + n, sizeof(body) - n, "<p>%s: %s</p>\r\n", longmsg, cause);
  n += snprintf(body + n, sizeof(body) - n, "<hr><em>The Tiny Web server</em>\r\n");

  size_t bodylen = (n < 0 || n >= (int)sizeof(body)) ? strlen("<html><body>Internal Error</body></html>")
                                                     : (size_t)n;

  if (n < 0 || n >= (int)sizeof(body))
  {
    strcpy(body, "<html><body>Internal Error</body></html>");
  }

  n = snprintf(buf, sizeof(buf), "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  n += snprintf(buf + n, sizeof(buf) - n, "Content-type: text/html\r\n");
  n += snprintf(buf + n, sizeof(buf) - n, "Content-length: %zu\r\n\r\n", bodylen);

  if (n < 0 || n >= (int)sizeof(buf))
  {
    char fallback[] =
        "HTTP/1.0 500 Internal Server Error\r\n"
        "Connection: close\r\n\r\n";
    Rio_writen(fd, fallback, strlen(fallback));
    return;
  }

  Rio_writen(fd, buf, n);
  Rio_writen(fd, body, bodylen);
}
