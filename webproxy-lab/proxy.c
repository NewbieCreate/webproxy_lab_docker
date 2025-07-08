#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000 /* 캐시의 최대 크기를 정의하는 매크로 (약 1MB) */
#define MAX_OBJECT_SIZE 102400 /* 캐시에 저장될 수 있는 단일 객체의 최대 크기를 정의하는 매크로 (약 100KB) */

/* You won't lose style points for including this long line in your code */
/* 프록시가 웹 서버에 요청을 보낼 때 사용할 User-Agent 헤더. 특정 브라우저(Firefox)인 것처럼 보이게 하여 호환성 문제를 줄입니다. */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

/* Function Prototypes - 앞으로 구현할 함수들의 원형(prototype) 선언 */
void doit(int fd);                                                                                              /* 클라이언트의 요청을 처리하는 주 함수 */
void parse_uri(char *uri, char *hostname, char *path, int *port);                                               /* URI를 파싱하여 호스트명, 경로, 포트를 추출하는 함수 */
void build_http_header(char *http_header, char *hostname, char *path, char *method, rio_t *client_rio); /* 서버로 보낼 HTTP 요청 헤더를 생성하는 함수 */
void *thread(void *vargp);                                                                                      /* 스레드가 실행할 함수 */

/* 프로그램의 시작점 */
int main(int argc, char **argv)
{
  int listenfd;                         /* 듣기 소켓의 파일 디스크립터 */
  int *connfd;                          /* 연결 소켓의 파일 디스크립터 포인터*/
  char hostname[MAXLINE], port[MAXLINE]; /* 클라이언트의 호스트명과 포트를 저장할 변수 */
  socklen_t clientlen;                  /* 클라이언트 주소 구조체의 크기를 저장할 변수 */
  struct sockaddr_storage clientaddr;   /* 클라이언트의 주소 정보를 저장할 구조체 */   /* 클라이언트의 주소 정보를 저장할 구조체 */
  pthread_t tid;                        /* 스레드 ID를 저장할 변수 */

  /* Check command-line arguments - 프로그램 실행 시 포트 번호를 인자로 받았는지 확인 */
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]); /* 인자가 2개가 아니면 사용법을 출력하고 종료 */
    exit(1);
  }

  /* Ignore SIGPIPE signals - SIGPIPE 시그널을 무시하도록 설정. */
  /* 연결이 끊긴 소켓에 데이터를 쓰려고 할 때 프로그램이 비정상 종료되는 것을 방지. */
  Signal(SIGPIPE, SIG_IGN);

  /* Open a listening socket on the specified port - 지정된 포트 번호(argv[1])로 들어오는 연결을 기다리는 듣기 소켓을 생성 */
  listenfd = Open_listenfd(argv[1]);

  /* Loop forever, accepting incoming connections - 무한 루프를 돌면서 클라이언트의 연결 요청을 계속 수락 */
  while (1)
  {
    clientlen = sizeof(clientaddr);                                          /* 클라이언트 주소 구조체의 크기를 설정 */
    connfd = Malloc(sizeof(int));                                            /* connfd에 동적 메모리 할당 */
    *connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);               /* 클라이언트의 연결 요청을 수락하고, 연결된 소켓(*connfd)을 생성 */
    
    /* 클라이언트 요청을 처리하기 위한 스레드 생성 (함수 포인터 형변환 추가) */
    Pthread_create(&tid, NULL, (void *(*)(void *))thread, connfd);
  }
  
  return 0; /* main 함수는 정상적으로는 이 라인에 도달하지 않습니다. */
}

/*
 * thread - 각 클라이언트 연결을 처리하기 위한 스레드 루틴.
 * vargp로부터 연결 파일 디스크립터를 받고, doit 함수를 호출하여 요청을 처리한 후,
 * 스레드 자신을 분리(detach)하고 파일 디스크립터를 닫습니다.
 */
void *thread(void *vargp)
{
    int connfd = *((int *)vargp); /* vargp에서 connfd를 역참조하여 가져옴 */
    Pthread_detach(pthread_self()); /* 스레드를 분리하여 자원 자동 해제 */
    Free(vargp);                    /* 동적으로 할당된 vargp 메모리 해제 */
    doit(connfd);                   /* 클라이언트 요청 처리 */
    Close(connfd);                  /* 연결 소켓 닫기 */
    return NULL;
}

/*
 * doit - 한 번의 HTTP 트랜잭션을 처리합니다.
 * 클라이언트로부터 요청 라인을 읽고 파싱하여 서버에 보낼 요청을 만들고,
 * 서버로부터 응답을 받아 클라이언트에게 전달합니다.
 */
void doit(int fd)
{
    int server_fd;                                    /* 서버와 연결될 소켓의 파일 디스크립터 */
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE]; /* 클라이언트 요청을 저장할 버퍼 및 변수들 */
    char hostname[MAXLINE], path[MAXLINE];            /* 파싱된 URI에서 호스트명과 경로를 저장할 변수 */
    int port;                                         /* 파싱된 URI에서 포트 번호를 저장할 변수 */
    char server_header[MAXLINE];                      /* 서버로 보낼 HTTP 헤더를 저장할 변수 */
    rio_t rio_client, rio_server;                     /* 클라이언트 및 서버와의 통신을 위한 RIO 구조체 */

    /* 클라이언트로부터 요청 읽기 */
    Rio_readinitb(&rio_client, fd);                   /* 클라이언트 소켓(fd)과 RIO 버퍼 초기화 */
    Rio_readlineb(&rio_client, buf, MAXLINE);         /* 클라이언트로부터 요청 라인 한 줄 읽기 */
    sscanf(buf, "%s %s %s", method, uri, version);    /* 요청 라인에서 메소드, URI, 버전을 파싱 */

    /* 현재는 GET 요청만 지원 */
    if (strcasecmp(method, "GET")) {                  /* 메소드가 "GET"이 아니면 (대소문자 무시) */
        printf("Proxy does not implement this method\n"); /* 에러 메시지 출력 */
        return;
    }

    /* URI 파싱하여 호스트명, 경로, 포트 추출 */
    parse_uri(uri, hostname, path, &port);

    /* 서버로 보낼 HTTP 헤더 생성 */
    build_http_header(server_header, hostname, path, method, &rio_client);

    /* 포트 번호를 문자열로 변환 */
    char port_str[10];
    sprintf(port_str, "%d", port);

    /* 최종 목적지 서버와 연결 */
    server_fd = Open_clientfd(hostname, port_str);    /* 호스트명과 포트번호로 서버에 연결 */
    if (server_fd < 0) {
        printf("Connection failed.\n");
        return;
    }

    /* 서버에 HTTP 요청 헤더 전송 */
    Rio_readinitb(&rio_server, server_fd);            /* 서버 소켓(server_fd)과 RIO 버퍼 초기화 */
    Rio_writen(server_fd, server_header, strlen(server_header)); /* 서버에 생성된 헤더 전송 */

    /* 서버로부터 응답을 받아 클라이언트에게 전송 */
    size_t n;
    while ((n = Rio_readlineb(&rio_server, buf, MAXLINE)) != 0) { /* 서버로부터 한 줄씩 응답 읽기 */
        Rio_writen(fd, buf, n);                       /* 읽은 응답을 클라이언트에게 그대로 전송 */
    }

    /* 서버 연결 닫기 */
    Close(server_fd);
}

/*
 * parse_uri - HTTP URI를 파싱합니다.
 * URI로부터 호스트명, 경로, 포트 번호를 추출합니다.
 */
void parse_uri(char *uri, char *hostname, char *path, int *port)
{
    /* 기본 포트는 80으로 설정 */
    *port = 80;
    char *ptr = strstr(uri, "//"); /* "http://" 다음 부분을 찾기 위해 "//" 검색 */
    ptr = (ptr != NULL) ? ptr + 2 : uri; /* "//"가 있으면 그 다음부터, 없으면 처음부터 시작 */

    char *port_ptr = strstr(ptr, ":"); /* 포트 번호를 찾기 위해 ":" 검색 */
    if (port_ptr) {
        *port_ptr = '\0'; /* 호스트명과 포트를 분리하기 위해 ":"를 NULL 문자로 변경 */
        sscanf(ptr, "%s", hostname); /* 호스트명 추출 */
        sscanf(port_ptr + 1, "%d%s", port, path); /* 포트 번호와 경로 추출 */
        if (strcmp(path, "") == 0) { /* 경로가 비어있으면 "/"로 설정 */
            strcpy(path, "/");
        }
    } else {
        char *path_ptr = strstr(ptr, "/"); /* 경로 시작을 찾기 위해 "/" 검색 */
        if (path_ptr) {
            *path_ptr = '\0'; /* 호스트명과 경로를 분리하기 위해 "/"를 NULL 문자로 변경 */
            sscanf(ptr, "%s", hostname); /* 호스트명 추출 */
            *path_ptr = '/'; /* NULL로 바꿨던 문자를 다시 "/"로 복원 */
            sscanf(path_ptr, "%s", path); /* 경로 추출 */
        } else {
            sscanf(ptr, "%s", hostname); /* 경로 없이 호스트명만 있는 경우 */
            strcpy(path, "/"); /* 기본 경로 "/" 설정 */
        }
    }
}

/*
 * build_http_header - 클라이언트의 요청을 기반으로 서버에 보낼 새로운 HTTP 요청 헤더를 생성합니다.
 */
void build_http_header(char *http_header, char *hostname, char *path, char *method, rio_t *client_rio)
{
    char buf[MAXLINE], request_hdr[MAXLINE], other_hdr[MAXLINE], host_hdr[MAXLINE];

    /* 요청 라인 생성 */
    sprintf(request_hdr, "%s %s HTTP/1.0\r\n", method, path);

    /* 클라이언트로부터 받은 나머지 헤더들을 읽고 처리 */
    while (Rio_readlineb(client_rio, buf, MAXLINE) > 0) {
        if (strcmp(buf, "\r\n") == 0) { /* 헤더의 끝을 만나면 중단 */
            break;
        }

        /* Host 헤더는 따로 저장 */
        if (!strncasecmp(buf, "Host", 4)) {
            strcpy(host_hdr, buf);
            continue;
        }
        
        /* User-Agent, Connection, Proxy-Connection 헤더는 무시하고 우리가 만든 값 사용 */
        if (strncasecmp(buf, "User-Agent", 10) && strncasecmp(buf, "Connection", 10) && strncasecmp(buf, "Proxy-Connection", 16)) {
            strcat(other_hdr, buf); /* 나머지 헤더들은 other_hdr에 추가 */
        }
    }

    /* Host 헤더가 없었다면 호스트명으로 직접 생성 */
    if (strlen(host_hdr) == 0) {
        sprintf(host_hdr, "Host: %s\r\n", hostname);
    }

    /* 모든 헤더들을 조합하여 최종 HTTP 요청 헤더 생성 */
    sprintf(http_header, "%s%s%sConnection: close\r\nProxy-Connection: close\r\n\r\n",
            request_hdr, host_hdr, other_hdr);
}