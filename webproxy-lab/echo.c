#include "csapp.h"

void echo(int connfd)
{
    size_t n;               // 수신된 바이트 수
    char buf[MAXLINE];      // 수신 버퍼
    rio_t rio;              // Robust I/O 구조체

    rio_readinitb(&rio, connfd);  // 연결 소켓 connfd에 대한 rio 버퍼 초기화

    // 클라이언트로부터 한 줄씩 읽어서 그대로 다시 보냄
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0)
    {
        printf("server received %d bytes\n", (int)n);  // 수신 바이트 수 출력
        rio_writen(connfd, buf, n);  // 그대로 클라이언트에게 다시 전송
    }
}