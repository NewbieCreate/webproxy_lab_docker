// /*
//  * adder.c - a minimal CGI program that adds two numbers together
//  */
// #include "csapp.h"                                // CS:APP에서 제공하는 유틸리티 함수 및 매크로 포함

// int main(void)
// {
//   char *buf, *p;                                  // 쿼리 문자열 포인터(buf), '&' 위치 포인터(p)
//   char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE]; // 인자 1, 인자 2, 출력할 콘텐츠용 버퍼
//   int n1 = 0, n2 = 0;                             // 숫자 인자 값 초기화

//   /* Extract the two arguments from QUERY_STRING */
//   if ((buf = getenv("QUERY_STRING")) != NULL)    // 환경변수 QUERY_STRING에서 입력 값 가져옴 (예: "n1=3&n2=5")
//   {
//     p = strchr(buf, '&');                         // '&' 문자를 찾아 두 인자를 구분
//     *p = '\0';                                     // '&'를 널 문자로 바꿔 첫 번째 인자만 남김 (buf는 이제 "n1=3")
//     strcpy(arg1, buf);                             // 첫 번째 인자(arg1)에 복사 ("n1=3")
//     strcpy(arg2, p + 1);                           // 두 번째 인자(arg2)에 복사 ("n2=5")
//     n1 = atoi(strchr(arg1, '=') + 1);              // '=' 다음 문자열부터 정수로 변환 (예: "3")
//     n2 = atoi(strchr(arg2, '=') + 1);              // '=' 다음 문자열부터 정수로 변환 (예: "5")
//   }

//   /* Create the HTML content string */
//   sprintf(content, "QUERY_STRING=%s\r\n<p>", buf);                    // 원래 쿼리 문자열 출력
//   sprintf(content + strlen(content), "Welcome to add.com: ");         // 인사 메시지 추가
//   sprintf(content + strlen(content), "THE Internet addition portal.\r\n<p>"); // 설명 추가
//   sprintf(content + strlen(content), "The answer is: %d + %d = %d\r\n<p>", 
//           n1, n2, n1 + n2);                                           // 계산 결과 출력
//   sprintf(content + strlen(content), "Thanks for visiting!\r\n");     // 마무리 메시지 출력

//   /* Send the HTTP response header */
//   printf("Content-type: text/html\r\n");                              // 응답의 MIME 타입을 HTML로 지정
//   printf("Content-length: %d\r\n", (int)strlen(content));             // 콘텐츠 길이를 명시 (브라우저가 알 수 있게)
//   printf("\r\n");                                                     // HTTP 헤더 종료 (빈 줄)

//   /* Send the response body */
//   printf("%s", content);                                              // 실제 HTML 본문 출력
//   fflush(stdout);                                                     // 출력 버퍼를 비워 즉시 전송

//   exit(0);                                                            // 프로그램 정상 종료
// }

#include "csapp.h"  // CS:APP robust I/O 라이브러리 포함

int main(void)
{
    char *query, *p;  // 원본 QUERY_STRING 포인터, '&' 구분자 포인터
    char query_cpy[MAXLINE];  // QUERY_STRING 복사본 (원본 손상 방지용)
    char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE]; // 인자 저장용 및 응답 출력용 버퍼
    int n1 = 0, n2 = 0;  // 정수 값 초기화

    // 인자 문자열 초기화
    strcpy(arg1, "");
    strcpy(arg2, "");

    // QUERY_STRING 환경변수 가져오기
    if ((query = getenv("QUERY_STRING")) != NULL) {
        // 복사해서 수정 (원본 getenv는 절대 직접 수정하지 않기!)
        strcpy(query_cpy, query);

        // '&' 기호가 있는지 확인
        p = strchr(query_cpy, '&');
        if (p != NULL) {
            *p = '\0';                  // '&'를 기준으로 두 인자로 나누기
            strcpy(arg1, query_cpy);    // 예: "a=30"
            strcpy(arg2, p + 1);        // 예: "b=50"
        } else {
            strcpy(arg1, query_cpy);    // 두 번째 인자가 없는 경우
            strcpy(arg2, "");           // 명시적으로 비워줘야 이전 값 유지 방지
        }

        // '=' 뒤의 숫자 추출 (안전하게 조건 체크)
        char *eq1 = strchr(arg1, '=');
        if (eq1 != NULL)
            n1 = atoi(eq1 + 1);
        else
            n1 = 0;

        char *eq2 = strchr(arg2, '=');
        if (eq2 != NULL)
            n2 = atoi(eq2 + 1);
        else
            n2 = 0;
    }

    // 응답 본문 HTML 생성
    sprintf(content, "QUERY_STRING=%s\r\n<p>", query ? query : "");
    sprintf(content + strlen(content), "Welcome to add.com: ");
    sprintf(content + strlen(content), "THE Internet addition portal.\r\n<p>");
    sprintf(content + strlen(content), "The answer is: %d + %d = %d\r\n<p>", n1, n2, n1 + n2);
    sprintf(content + strlen(content), "Thanks for visiting!\r\n");

    // HTTP 응답 헤더 출력
    printf("Content-type: text/html\r\n");
    printf("Content-length: %lu\r\n", strlen(content));
    printf("\r\n");

    // HTTP 응답 바디 출력
    printf("%s", content);
    fflush(stdout);  // 출력 버퍼 강제 비움

    exit(0);  // 정상 종료
}