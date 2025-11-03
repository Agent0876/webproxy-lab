/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void)
{
  char *buf, *p;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
  size_t len;
  int n1 = 0, n2 = 0;

  /* Extract the two arguments */
  if ((buf = getenv("QUERY_STRING")) != NULL)
  {
    p = strchr(buf, '&');
    *p = '\0';
    strcpy(arg1, buf);
    strcpy(arg2, p + 1);
    n1 = atoi(strchr(arg1, '=') + 1);
    n2 = atoi(strchr(arg2, '=') + 1);
  }

  /* Make the response body */
  snprintf(content, sizeof(content), "QUERY_STRING=%s\r\n<p>", buf);
  len = strlen(content);
  snprintf(content + len, sizeof(content) - len, "Welcome to add.com: ");
  len = strlen(content);
  snprintf(content + len, sizeof(content) - len,
           "THE Internet addition portal.\r\n<p>");
  len = strlen(content);
  snprintf(content + len, sizeof(content) - len,
           "The answer is: %d + %d = %d\r\n<p>", n1, n2, n1 + n2);
  len = strlen(content);
  snprintf(content + len, sizeof(content) - len,
           "Thanks for visiting!\r\n");

  /* Generate the HTTP response */
  printf("Content-type: text/html\r\n");
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("\r\n");
  printf("%s", content);
  fflush(stdout);

  exit(0);
}
/* $end adder */
