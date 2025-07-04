#include "csapp.h"  // CS:APP 책에서 제공하는 소켓 및 I/O 관련 헬퍼 함수들 포함

int main(int argc, char **argv) {
    int clintfd;                   // 클라이언트 소켓 파일 디스크립터
    char *host, *port, buf[MAXLINE];  // 호스트 주소, 포트번호, 입출력 버퍼
    rio_t rio;                     // Robust I/O를 위한 rio_t 구조체

    // 인자 개수가 정확하지 않으면 에러 메시지 출력 후 종료
    if (argc != 3){
        fprintf(stderr, "usage : %s <host> <port> \n", argv[0]);
        exit(0);
    }
    host = argv[1];  // 호스트 이름 (예: "localhost" 또는 IP)
    port = argv[2];  // 포트 번호 (예: "8000")

    clintfd = Open_clientfd(host, port); // 서버와의 연결을 위한 소켓 열기
    rio_readinitb(&rio, clintfd);        // rio 버퍼를 clintfd로 초기화

    // 표준 입력에서 한 줄씩 읽어서 서버로 전송, 서버 응답 출력
    while(Fgets(buf, MAXLINE, stdin) != NULL){
        rio_writen(clintfd, buf, strlen(buf));  // 입력된 줄을 서버로 전송
        rio_readlineb(&rio, buf, MAXLINE);      // 서버로부터 응답 수신
        Fputs(buf, stdout);                     // 응답을 stdout으로 출력
    }

    close(clintfd);  // 소켓 닫기
    exit(0);         // 정상 종료
}