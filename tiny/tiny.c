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
    // 잘못된 사용법 출력 후 종료
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  // 서버 소켓 열기
  listenfd = Open_listenfd(argv[1]);

  // 무한 루프를 돌며 클라이언트 연결 처리
  while (1)
  {
    // 클라이언트 연결 수락
    clientlen = sizeof(clientaddr); // 클라이언트 주소 길이 초기화
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen); // 클라이언트 연결 수락
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0); // 클라이언트 호스트 이름과 포트 번호 얻기
    printf("Accepted connection from (%s, %s)\n", hostname, port); // 연결 정보 출력
    doit(connfd); // 클라이언트 요청 처리
    Close(connfd); // 클라이언트 연결 닫기
  }
}

void doit(int fd)
{
  // fd: 클라이언트 연결 디스크립터
  int is_static; // 정적 콘텐츠 여부
  struct stat sbuf; // 파일 상태 정보 구조체
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE]; // 요청 라인 정보
  char filename[MAXLINE], cgiargs[MAXLINE]; // 파일 이름과 CGI 인자
  rio_t rio; // RIO 버퍼 구조체

  Rio_readinitb(&rio, fd); // RIO 버퍼 초기화
  Rio_readlineb(&rio, buf, MAXLINE); // 요청 라인 읽기
  printf("Request headers:\n"); // 요청 헤더 출력
  printf("%s", buf); // 요청 라인 출력
  sscanf(buf, "%s %s %s", method, uri, version); // 요청 라인 파싱

  if (strcasecmp(method, "GET")) // GET 메서드가 아닌 경우
  {
    clienterror(fd, method, "501", "Not Implemented", "Tiny does not implement this method"); // 에러 응답 전송
    return;
  }

  read_requesthdrs(&rio); // 요청 헤더 읽기

  is_static = parse_uri(uri, filename, cgiargs); // URI 파싱

  if (stat(filename, &sbuf) < 0) // 파일 상태 정보 얻기 실패
  {
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file"); // 에러 응답 전송
    return;
  }

  if (is_static) // 정적 콘텐츠인 경우
  {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) // 파일이 정규 파일이 아니거나 읽기 권한이 없는 경우
    {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file"); // 에러 응답 전송
      return;
    }
    serve_static(fd, filename, sbuf.st_size); // 정적 콘텐츠 제공
  }
  else
  {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) // 파일이 정규 파일이 아니거나 실행 권한이 없는 경우
    {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program"); // 에러 응답 전송
      return;
    }
    serve_dynamic(fd, filename, cgiargs); // 동적 콘텐츠 제공
  }
}

void read_requesthdrs(rio_t *rp) // 요청 헤더 읽기 함수
{
  // rp : RIO 버퍼 구조체 포인터
  char buf[MAXLINE]; // 버퍼 선언

  Rio_readlineb(rp, buf, MAXLINE); // 첫 번째 헤더 라인 읽기
  printf("%s", buf); // 헤더 라인 출력
  while (strcmp(buf, "\r\n")) // 빈 줄이 나올 때까지 반복
  {
    Rio_readlineb(rp, buf, MAXLINE); // 다음 헤더 라인 읽기
    printf("%s", buf); // 헤더 라인 출력
  }
  return;
}

int parse_uri(char *uri, char *filename, char *cgiargs) // URI 파싱 함수
{
  // uri : 요청 URI
  // filename : 파일 이름 버퍼
  // cgiargs : CGI 인자 버퍼
  char *ptr; // 포인터 선언

  if (!strstr(uri, "cgi-bin")) // 정적 콘텐츠인 경우
  {
    strcpy(cgiargs, ""); // CGI 인자 초기화
    strcpy(filename, "."); // 현재 디렉토리로 시작
    strcat(filename, uri); // URI를 파일 이름에 추가
    if (uri[strlen(uri) - 1] == '/') // URI가 '/'로 끝나는 경우
      strcat(filename, "home.html"); // 기본 파일 이름 추가
    return 1; // 정적 콘텐츠 반환
  }
  else
  {
    ptr = strchr(uri, '?'); // '?' 문자 찾기
    if (ptr) // CGI 인자가 있는 경우
    {
      strcpy(cgiargs, ptr + 1); // CGI 인자 복사
      *ptr = '\0'; // URI에서 '?' 이후 부분 제거
    }
    else
      strcpy(cgiargs, ""); // CGI 인자 초기화
    strcpy(filename, "."); // 현재 디렉토리로 시작
    strcat(filename, uri); // URI를 파일 이름에 추가
    return 0;
  }
}

void serve_static(int fd, char *filename, int filesize) // 정적 콘텐츠 제공
{
  // fd : 클라이언트 연결 디스크립터
  // filename : 제공할 파일 이름
  // filesize : 파일 크기
  int srcfd; // 소스 파일 디스크립터
  char *srcp, filetype[MAXLINE], buf[MAXBUF]; // 파일 타입과 버퍼
  int n = 0; // 버퍼 길이

  get_filetype(filename, filetype); // 파일 타입 얻기

  n = snprintf(buf, sizeof(buf), "HTTP/1.0 200 OK\r\n"); // 응답 라인 작성
  n += snprintf(buf + n, sizeof(buf) - n, "Server: Tiny Web Server\r\n"); // 서버 헤더 작성
  n += snprintf(buf + n, sizeof(buf) - n, "Connection: close\r\n"); // 연결 헤더 작성
  n += snprintf(buf + n, sizeof(buf) - n, "Content-length: %d\r\n", filesize); // 콘텐츠 길이 헤더 작성
  n += snprintf(buf + n, sizeof(buf) - n, "Content-type: %s\r\n\r\n", filetype); // 콘텐츠 타입 헤더 작성

  if (n < 0 || n >= (int)sizeof(buf)) // 버퍼 오버플로우 검사
  {
    char fallback[] =
        "HTTP/1.0 500 Internal Server Error\r\n"
        "Connection: close\r\n\r\n"; // 대체 응답 작성
    Rio_writen(fd, (void *)fallback, strlen(fallback)); // 대체 응답 전송
    return;
  }

  Rio_writen(fd, buf, n); // 응답 헤더 전송

  srcfd = Open(filename, O_RDONLY, 0); // 파일 열기
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); // 파일 매핑
  Close(srcfd); // 파일 디스크립터 닫기
  Rio_writen(fd, srcp, filesize); // 파일 내용 전송
  Munmap(srcp, filesize); // 매핑 해제
}

void get_filetype(char *filename, char *filetype) // 파일 타입 얻기 함수
{
  // filename : 파일 이름
  // filetype : 파일 타입 버퍼
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else
    strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs) // 동적 콘텐츠 제공
{
  // fd : 클라이언트 연결 디스크립터
  // filename : CGI 프로그램 파일 이름
  // cgiargs : CGI 인자
  char buf[MAXLINE], *emptylist[] = {NULL}; // 빈 인자 리스트

  sprintf(buf, "HTTP/1.0 200 OK\r\n"); // 응답 라인 작성
  Rio_writen(fd, buf, strlen(buf)); // 응답 라인 전송
  sprintf(buf, "Server: Tiny Web Server\r\n"); // 서버 헤더 작성
  Rio_writen(fd, buf, strlen(buf)); // 서버 헤더 전송

  if (Fork() == 0) // 자식 프로세스 생성
  {
    setenv("QUERY_STRING", cgiargs, 1); // CGI 인자 환경 변수 설정
    Dup2(fd, STDOUT_FILENO); // 표준 출력 리다이렉션
    Execve(filename, emptylist, environ); // CGI 프로그램 실행
  }
  Wait(NULL); // 자식 프로세스 대기
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) // 클라이언트 에러 응답 함수
{
  // fd : 클라이언트 연결 디스크립터
  // cause : 에러 원인
  // errnum : 에러 번호
  // shortmsg : 짧은 에러 메시지
  // longmsg : 긴 에러 메시지
  char buf[MAXLINE], body[MAXBUF]; // 버퍼 선언
  int n = 0; // 버퍼 길이

  n = snprintf(body, sizeof(body), "<html><title>Tiny Error</title>"); // HTML 본문 시작
  n += snprintf(body + n, sizeof(body) - n, "<body bgcolor=\"ffffff\">\r\n"); // 배경색 설정
  n += snprintf(body + n, sizeof(body) - n, "%s: %s\r\n", errnum, shortmsg); // 에러 번호와 짧은 메시지
  n += snprintf(body + n, sizeof(body) - n, "<p>%s: %s</p>\r\n", longmsg, cause); // 긴 메시지와 원인
  n += snprintf(body + n, sizeof(body) - n, "<hr><em>The Tiny Web server</em>\r\n"); // 푸터

  size_t bodylen = (n < 0 || n >= (int)sizeof(body)) ? strlen("<html><body>Internal Error</body></html>")
                                                     : (size_t)n; // 본문 길이 계산

  if (n < 0 || n >= (int)sizeof(body)) // 본문 작성 실패 시 대체 본문 작성
  {
    strcpy(body, "<html><body>Internal Error</body></html>"); // 대체 본문
  }

  n = snprintf(buf, sizeof(buf), "HTTP/1.0 %s %s\r\n", errnum, shortmsg); // 응답 라인 작성
  n += snprintf(buf + n, sizeof(buf) - n, "Content-type: text/html\r\n"); // 콘텐츠 타입 헤더 작성
  n += snprintf(buf + n, sizeof(buf) - n, "Content-length: %zu\r\n\r\n", bodylen); // 콘텐츠 길이 헤더 작성

  if (n < 0 || n >= (int)sizeof(buf)) // 버퍼 오버플로우 검사
  {
    char fallback[] =
        "HTTP/1.0 500 Internal Server Error\r\n"
        "Connection: close\r\n\r\n"; // 대체 응답 작성
    Rio_writen(fd, fallback, strlen(fallback)); // 대체 응답 전송
    return;
  }

  Rio_writen(fd, buf, n); // 응답 헤더 전송
  Rio_writen(fd, body, bodylen); // 응답 본문 전송
}
