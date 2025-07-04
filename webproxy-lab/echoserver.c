#include "csapp.h"

void echo(int connfd);  // 클라이언트 요청을 받아 처리하는 함수 (별도로 정의되어야 함)

int main(int argc, char **argv){
    int listenfd, connfd;                        // 듣기 및 연결 소켓 디스크립터
    socklen_t clientlen;                         // 클라이언트 주소 구조체 크기
    struct sockaddr_storage clientaddr;          // 클라이언트 주소 정보 저장
    char client_hostname[MAXLINE], client_port[MAXLINE];  // 클라이언트 호스트/포트 문자열

    // 포트 번호가 명시되지 않았을 경우 에러 처리
    if (argc != 2){
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]);  // 서버 소켓 열고 포트에 바인딩
    while (1) {
        clientlen = sizeof(struct sockaddr_storage);  // 클라이언트 주소 길이 초기화

        // 클라이언트의 연결을 수락하고 새로운 연결 소켓 생성
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

        // 연결된 클라이언트의 호스트명과 포트 번호를 문자열로 변환
        Getnameinfo((SA *) &clientaddr, clientlen,
                    client_hostname, MAXLINE,
                    client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);

        echo(connfd);     // 클라이언트와의 에코 통신 처리
        Close(connfd);    // 클라이언트 연결 종료
    }

    exit(0);
}