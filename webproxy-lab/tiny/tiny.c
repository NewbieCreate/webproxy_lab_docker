/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"
#include <signal.h>

void doit(int fd);  
// 클라이언트의 HTTP 요청을 처리하는 메인 함수 (정적/동적 콘텐츠 판단 및 응답)

void read_requesthdrs(rio_t *rp, int *content_length);  
// 요청 헤더를 읽고 무시 (단순히 읽어들이기만 하고 분석은 안 함)

int parse_uri(char *uri, char *filename, char *cgiargs);  
// URI를 파싱해서 정적 콘텐츠인지 동적 콘텐츠인지 판단하고, 해당 파일 경로와 CGI 인자 추출

void serve_static(int fd, char *filename, int filesize);  
// 정적 콘텐츠 (HTML, 이미지 등)를 클라이언트에게 전송

void get_filetype(char *filename, char *filetype);  
// 요청된 파일의 확장자를 보고 MIME 타입 결정 (ex: .html → text/html)

void serve_dynamic(int fd, char *filename, char *cgiargs,char *post_data, int content_length, int is_post);  
// 동적 콘텐츠 (CGI 프로그램 실행 결과)를 클라이언트에게 전달

void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);  
// 클라이언트 요청에 문제가 있을 때, 에러 메시지를 HTML 형태로 전송

void sigchld_handler(int sig);
// 자식 프로세스가 종료되었을 때 발생하는 SIGCHLD 시그널을 처리하는 핸들러 함수

int main(int argc, char **argv)
{
  int listenfd, connfd;                                  // 서버 소켓, 클라이언트와의 연결 소켓 디스크립터
  char hostname[MAXLINE], port[MAXLINE];                 // 클라이언트의 호스트 이름과 포트 번호 저장용
  socklen_t clientlen;                                   // 클라이언트 주소 구조체 크기 저장
  struct sockaddr_storage clientaddr;                    // 클라이언트 주소 정보 저장 (IPv4/IPv6 모두 호환)

  // 메인 함수 내에서 SIGCHLD 시그널이 발생할 때 sigchld_handler를 호출하도록 등록
  // 이로써 CGI 자식 프로세스가 종료될 때 좀비가 되지 않고 자동으로 수확됨
  signal(SIGPIPE, SIG_IGN);

  /* Check command line args */
  if (argc != 2)                                          // 포트 번호가 명령행 인자로 안 들어왔으면
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);       // 사용 방법 출력
    exit(1);                                              // 비정상 종료
  }

  listenfd = Open_listenfd(argv[1]);                      // 듣기 소켓 열기 (argv[1] = 포트 번호)
  while (1)                                               // 무한 루프: 클라이언트 요청 계속 처리
  {
    clientlen = sizeof(clientaddr);                       // 클라이언트 주소 구조체 크기 초기화
    connfd = Accept(listenfd, (SA *)&clientaddr,          // 클라이언트 연결 수락, connfd는 통신용 소켓
                    &clientlen); // line:netp:tiny:accept

    Getnameinfo((SA *)&clientaddr, clientlen,             // 클라이언트 호스트명과 포트 문자열 얻기
                hostname, MAXLINE, port, MAXLINE, 0);

    printf("Accepted connection from (%s, %s)\n",         // 접속한 클라이언트 정보 출력
           hostname, port);

    doit(connfd);  // line:netp:tiny:doit                  // 요청 처리 함수 호출 (HTTP 응답 처리 포함)
    Close(connfd); // line:netp:tiny:close                 // 클라이언트와 연결 종료
  }
}
/*클라이언트의 HTTP 요청을 처리하는 메인 함수 (정적/동적 콘텐츠 판단 및 응답)*/
void doit(int fd)
{
  int is_static;                                          // 정적 콘텐츠 여부 판단 플래그
  struct stat sbuf;                                       // 파일 상태정보 저장 구조체
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE]; // 요청 헤더 파싱용 버퍼
  char filename[MAXLINE], cgiargs[MAXLINE];                           // 파일 이름 및 cgi 인자 저장용 버퍼
  char post_data[MAXBUF];
  int content_length = 0;
  rio_t rio;                                                          // robust I/O 구조체

  /* 요청 읽는 라인*/
  Rio_readinitb(&rio, fd);                                           //robust I/O 초기화
  Rio_readlineb(&rio, buf, MAXLINE);                                 // 첫줄 읽기 :ex) "GET /index.html HTTP/1.1"
  printf("Request headers : \n %s", buf);                                 
  sscanf(buf, "%s %s %s", method, uri, version);
  
  int is_post = 0;
  if (strcasecmp(method, "POST") == 0) {
    is_post = 1;
  }
  else if(strcasecmp(method, "GET") != 0){
    clienterror(fd, method, "501", "NOT implemented", "Tiny does not impement this method");
    return;
  }
  // if(strcasecmp(method, "GET") != 0 && strcasecmp(method, "POST") != 0){    // GET이 아닐 경우 거나 POST가 아닐경우 처리 불가
  //   clienterror(fd, method, "501", "NOT implemented", "Tiny does not impement this method");
  //   return;                                                         //method가 Get이 아니면 오류 응답후 종료
  // }
  read_requesthdrs(&rio, &content_length);                                           // 나머지 요청 헤더 무시하며 읽기
  if (is_post && content_length > 0){
    Rio_readnb(&rio, post_data, content_length);
    post_data[content_length] = '\0';
  }
  /* URI 파싱 -> filename, cgiarge 분리 + 정적/동적 여부 판단 */
  is_static = parse_uri(uri, filename, cgiargs);  // uri 분석 : 정적 or 동적결정, 파일이름 & 인자추출
  if(stat(filename, &sbuf) <0){                   //해당 파일이 존재 하지 않으면
    clienterror(fd, filename, "404", "NOT Found", "Tiny couldn't find this file");
    return;                                      // 오류 출력후 종료
  }
  if(is_static) {/* 서버 정적 컨텐츠 요청 처리*/
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)){ //일반 파일인지 + 실행 권한 있는지 확인
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
    serve_static(fd, filename, sbuf.st_size);                  //정적 콘텐츠 클라이언트에 전송
  }else{ /* 서버 동적 컨텐츠 요청 처리*/
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {  //일반 파일인지 + 실행권한이 있는지 확인 
      clienterror(fd, filename, "403", "Forvidden", "Tiny couldn't read the file");
      return;
    }
    serve_dynamic(fd, filename, cgiargs, post_data, content_length, is_post);  // CGI 프로그램 실행 및 결과전송
  }
}
// 자식 프로세스가 종료되었을 때 발생하는 SIGCHLD 시그널을 처리하는 핸들러 함수
void sigchld_handler(int sig){
   // 종료된 자식 프로세스를 블로킹 없이 수확
    // -1: 어떤 자식이든 상관없음
    // NULL: 자식 종료 상태는 저장하지 않음
    // WNOHANG: 자식이 종료되지 않았으면 바로 반환
  while(waitpid(-1, NULL, WNOHANG) >0);
}
// 클라이언트 요청에 문제가 있을 때, 에러 메시지를 HTML 형태로 전송
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF]; //응답 헤더와 바디을 담을 버퍼

  /* HTTP 응답 바디 생성(HTML 형식으로 구성) */
  sprintf(body, "<html><title>tiny error</title>");              //HTML 제목
  sprintf(body, "%s<body bgcolor =""ffff"">\r\n", body);         //배경색 지정
  sprintf(body, "%s%s : %s \r\n", body, errnum, shortmsg);       //상태 코드와 메세지 출력
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);        // 자세한 오류 원인 출력
  sprintf(body, "%s<hr><em>The Tiny Web Server</em>\r\n", body);  // 서버 이름

  /* HTTP 요청 출력 */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg); // 상태 줄(ex :  404 Not Found)
  Rio_writen(fd, buf, strlen(buf));                        // 클라이언트에 전송
  sprintf(buf, "Content-Type: text/html\r\n");            // MIME 타입
  Rio_writen(fd, buf, strlen(buf));                        
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body)); //바디 길이
  Rio_writen(fd, buf, strlen(buf));

  /* HTTP 응답 바디 전송 */
  Rio_writen(fd, body, strlen(body)); // HTML 본문에 전송
}
// // 요청 헤더를 읽고 무시 (단순히 읽어들이기만 하고 분석은 안 함)
// void read_requesthdrs(rio_t *rp)      
// {
//   char buf[MAXLINE];                         //요청 헤더를 임시로 저장할 버퍼

//   Rio_readlineb(rp, buf, MAXLINE);           //첫 번쨰 헤더 라인 읽기
//   while(strcmp(buf, "\r\n")) {               // 빈줄("\r\n")이 나올때 까지 반복
//     Rio_readlineb(rp, buf, MAXLINE);         // 다음줄 읽기
//     printf("%s", buf);                       // 다음줄 출력
//   }
//   return;                                    // 헤더읽기 종료
// }

/* 전부 에코 할 수 있게 하는 명령 */
void read_requesthdrs(rio_t *rp, int *content_length)
{
  char buf[MAXLINE];
  *content_length = 0;
  while(Rio_readlineb(rp,buf,MAXLINE) > 0) {
    printf("%s", buf);
    if(strcasecmp(buf, "\r\n") == 0){
      break;
    }
    if(strncasecmp(buf, "Content-Length:", 15) == 0){
      *content_length = atoi(buf + 15);
    }
  }
}



/* URI를 파싱하여 요청이 정적 콘텐츠인지 동적 콘텐츠인지 판별하고,
   파일 경로(filename)와 CGI 인자(cgiargs)를 추출한다.
   정적 콘텐츠 → return 1, 동적 콘텐츠 → return 0 */
int parse_uri(char *uri, char *filename, char *cgiargs)
{
    char *ptr;

    /* 정적 콘텐츠인 경우: URI에 "cgi-bin"이 포함되지 않음 */
    if (!strstr(uri, "cgi-bin")) {
        strcpy(cgiargs, "");         // 정적 콘텐츠는 CGI 인자 없음
        strcpy(filename, ".");       // 상대 경로 시작 → "./"
        strcat(filename, uri);       // URI를 이어붙여 "./index.html" 또는 "./"

        /* URI가 '/'로 끝나면 기본 파일 이름으로 "home.html" 설정 */
        if (uri[strlen(uri) - 1] == '/') {
            strcat(filename, "home.html");
        }

        return 1; // 정적 콘텐츠임을 나타냄
    }

    /* 동적 콘텐츠인 경우: URI에 "cgi-bin"이 포함됨 */
    else {
        ptr = index(uri, '?');       // '?' 문자 탐색 → CGI 인자의 시작 위치
        if (ptr) {
            strcpy(cgiargs, ptr + 1); // '?' 이후 문자열을 CGI 인자에 저장
            *ptr = '\0';              // '?' 위치를 null 문자로 바꿔 URI만 남김
        } else {
            strcpy(cgiargs, "");     // 인자가 없는 경우 빈 문자열
        }

        strcpy(filename, ".");       // 상대 경로 시작 → "./"
        strcat(filename, uri);       // URI를 이어붙여 "./cgi-bin/xxx"

        return 0; // 동적 콘텐츠임을 나타냄
    }
}

/*
성능이 중요하고, 큰 파일을 빠르게 제공해야 한다면 → mmap
코드 이해와 디버깅, 교육용 혹은 범용성이 중요하면 → read + rio_writen
 */

// 정적 콘텐츠를 클라이언트에게 전송하는 함수 (Mmap 방식)
// void serve_static(int fd, char *filename, int filesize)
// {
//   int srcfd;                                           //파일 디스크립터
//   char *srcp, filetype[MAXLINE], buf[MAXBUF];          // 파일을 memory-map할 포인터, 헤더 버퍼 등

//   /* 클린트에 요청한 내용을 보낸다 */
//   get_filetype(filename, filetype);                   // 파일 확장자에 따라 mime 타입 결정
//   sprintf(buf, "HTTP/1.0 200 OK\r\n");                // 상태 줄
//   sprintf(buf, "%sServer : Tiny Web Server \r\n", buf);       // 서버정보
//   sprintf(buf, "%sConnection : close\r\n",buf);               // 연결종료 명시
//   sprintf(buf, "%sContent-length : %d\r\n",buf, filesize);    // 콘텐츠 길이
//   sprintf(buf, "%sContent-type : %s\r\n\r\n", buf, filetype); // mime 타입
//   Rio_writen(fd, buf, strlen(buf));                           // 클라이언트에 헤더 전송
//   printf("Response headers:\n");
//   printf("%s", buf);                                          // 디버깅용 헤더 출력

//     /* 파일을 열고 메모리에 매핑한 뒤, 그대로 클라이언트로 전송 */
//   srcfd = Open(filename, O_RDONLY, 0);                        //읽기 전용으로 파일 열기
//   srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); // 메모리에 매핑
//   Close(srcfd);                                               // 매핑했으므로 파일은 바로 닫아도됨
//   Rio_writen(fd, srcp, filesize);                             // 매핑된 내용을 클라이언트에게 전송
//   Munmap(srcp, filesize);                                     // 매핑해제
// }

/* read + rio_writen 방식 */
void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;                                                   //요청된 파일을 열기 위한 파일 디스크립터
  char buf[MAXBUF];                                            // 파일 내용을 담을 버퍼
  char filetype[MAXLINE], hdrbuf[MAXBUF];                      // MIME 타입과 HTTP 응답 헤더를 담을 버퍼

  /* 요청된 파일의 확장자에 따라 MIME 타입 결정 */
  get_filetype(filename, filetype);

  /* 응답 헤더 문자열 생성 */
  sprintf(hdrbuf, "HTTP/1.0 200 Ok \r\n");                              //상태줄
  sprintf(hdrbuf, "%sServer: Tiny Web Server\r\n",hdrbuf);              // 서버정보
  sprintf(hdrbuf, "%sConnection : Close\r\n",hdrbuf);                   // 연결종료 명시
  sprintf(hdrbuf, "%sContent-length : %d\r\n",hdrbuf,filesize);         // 콘텐츠길이
  sprintf(hdrbuf, "%sContent-type : %s\r\n\r\n",hdrbuf,filetype);           // MIME 타입
  
  /* 응답 헤더를 클라이언트에게 전송 */
  Rio_writen(fd, hdrbuf, strlen(hdrbuf));                              //헤더전송
  printf("Response Headers:\n%s",hdrbuf);                              //서버 콘솔에 헤더 출력

  /* 정적콘텐츠 파일을 열고 내용을 읽어 클라이언트에게 전송 */
  srcfd = open(filename, O_RDONLY, 0);                                 //읽기 전용으로 파일열기
  ssize_t n;                          
  while ((n = read(srcfd, buf, MAXBUF)) > 0){                         // 파일을 MAXBUF 바이트씩 읽는다.
    Rio_writen(fd,buf, n);                                            // 읽은 만큼 클라이언트에게 전송
  }
  Close(srcfd);                                                       // 파일 디스크립터 닫기
}


// 요청된 파일 이름의 확장자를 보고 MIME 타입 결정
void get_filetype(char *filename, char *filetype)
{
    if (strstr(filename, ".html"))                  // HTML 문서
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".css"))              // CSS 스타일시트
        strcpy(filetype, "text/css");
    else if (strstr(filename, ".js"))               // JavaScript 파일
        strcpy(filetype, "application/javascript");
    else if (strstr(filename, ".gif"))              // GIF 이미지
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".jpg") || strstr(filename, ".jpeg")) // JPEG 이미지
        strcpy(filetype, "image/jpeg");
    else if (strstr(filename, ".png"))              // PNG 이미지
        strcpy(filetype, "image/png");
    else if (strstr(filename, ".mpg") || strstr(filename, ".mpeg")) // MPEG 비디오
        strcpy(filetype, "video/mpeg");
    else if (strstr(filename, ".mp4"))              // MP4 비디오
        strcpy(filetype, "video/mp4");
    else if (strstr(filename, ".webm"))             // WebM 비디오
        strcpy(filetype, "video/webm");
    else if (strstr(filename, ".ogg"))              // Ogg 비디오
        strcpy(filetype, "video/ogg");
    else if (strstr(filename, ".mp3"))              // MP3 오디오
        strcpy(filetype, "audio/mpeg");
    else if (strstr(filename, ".wav"))              // WAV 오디오
        strcpy(filetype, "audio/wav");
    else if (strstr(filename, ".pdf"))              // PDF 문서
        strcpy(filetype, "application/pdf");
    else                                            // 기타: 바이너리 형태로 처리
        strcpy(filetype, "application/octet-stream");
}
// CGI 프로그램을 실행하고 결과를 클라이언트에게 전달하는 함수
// void serve_dynamic(int fd, char *filename, char *cgiargs,char *post_data, int content_length, int is_post)
// {
//   char buf[MAXLINE], *emptylist[] = {NULL}; // CGI 인자 없음 → 빈 인자 리스트 생성
 
 
//   /* HTTP 응답 헤더 전송 (본문은 CGI가 직접 출력함) */
//   sprintf(buf, "HTTP/1.0 200 OK \r\n"); // 상태 줄 전송
//   Rio_writen(fd, buf, strlen(buf)); // 클라이언트로 전송
//   sprintf(buf, "Server: Tiny Web Server \r\n"); // 서버 정보 헤더
//   Rio_writen(fd, buf, strlen(buf));// 클라이언트로 전송

//   if(Fork() == 0) { // 자식 프로세스 생성
//     setenv("QUERY_STRING", cgiargs, 1); // 환경변수로 CGI 인자 설정
//     Dup2(fd, STDOUT_FILENO); // 표준 출력(STDOUT)을 클라이언트 소켓으로 리다이렉트
//     Execve(filename, emptylist, environ); // CGI 프로그램 실행 (출력이 곧 클라이언트 응답)
//   }
//   wait(NULL);  // 부모는 자식 종료까지 대기
// }

void serve_dynamic(int fd, char *filename, char *cgiargs, char *post_data, int content_length, int is_post)
{
  char buf[MAXLINE], *empylist[] = {NULL};

   /* 응답헤더 전송 */
   sprintf(buf, "HTTP/1.0 200 OK\r\n");
   Rio_writen(fd, buf, strlen(buf));
   sprintf(buf, "Server : Tiny Web Server\r\n");
   Rio_writen(fd, buf, strlen(buf));

   if(Fork() == 0) {
    setenv("REQUEST_METHOD", is_post ? "POST" : "GET", 1);
    setenv("QUERY_STRING",cgiargs, 1);
    if(is_post){
      char clen[16];
      sprintf(clen, "%d", content_length);
      setenv("CONTENT_LENGTH", clen, 1);

      int pipefd[2];
      pipe(pipefd);
      write(pipefd[1], post_data, content_length);
      close(pipefd[1]);
      Dup2(pipefd[0], STDIN_FILENO);
      close(pipefd[0]);
    }
    Dup2(fd, STDOUT_FILENO);
    Execve(filename, empylist, environ);
   }
   Wait(NULL);
}