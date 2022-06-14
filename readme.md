# 2022_tcpip
- 2022년 1학기 TCP/IP 네트워크 프로그래밍 저장소

# 2022-06-15 기말고사 
- 멀티쓰레드 기반의 다중접속 서버의 구현 14주차 강의 내용에 있는 코드입니다.
- 쓰레드 기반 TCP/IP 네트워크 프로그래밍 소스 주석한 코드입니다.

## chat_serv.c

```c

#include <stdio.h> // 입출력
#include <stdlib.h> // 문자열 변환, 의사 난수 생성
#include <unistd.h> // 표준 심볼 상수 및 자료형
#include <string.h> // 문자열 상수
#include <arpa/inet.h> // 주소변환
#include <sys/socket.h> // 소켓 연결
#include <netinet/in.h> // IPv4 전용 기능
#include <pthread.h> // 쓰레드 사용

#define BUF_SIZE 100
#define MAX_CLNT 256

void* handle_clnt(void* arg); // 클라이언트로 부터 받은 메시지를 처리
void send_msg(char* msg, int len); // 클라이언트로 부터 메시지를 받아옴
void error_handling(char* msg); // 에러 처리

int clnt_cnt = 0; // 서버에 접속한 클라이언트 수 카운팅
int clnt_socks[MAX_CLNT]; // 클라이언트와의 송수신을 위해 생성한 소켓의 파일 디스크립터를 저장한 배열
pthread_mutex_t mutx; // 뮤텍스를 통한 쓰레드 동기화를 위한 변수

int main(int argc, char* argv[])
{
	int serv_sock, clnt_sock; // 서버소켓, 클라이언트 소켓 변수 선언
	struct sockaddr_in serv_adr, clnt_adr; // 서버, 클라이언트 주소 변수 선언
	int clnt_adr_sz; // 클라이언트 주소 크기
	pthread_t t_id; // 쓰레드 아이디
	if (argc != 2) { // 실행파일 경로/PORT번호를 입력으로 받아야 함
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

	pthread_mutex_init(&mutx, NULL); // 뮤텍스 생성
	serv_sock = socket(PF_INET, SOCK_STREAM, 0); // TCP 소켓 생성

	memset(&serv_adr, 0, sizeof(serv_adr)); // 서버 주소정보 초기화
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));

	if (bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) // 서버 주소정보를 기반으로 주소할당
		error_handling("bind() error");
	if (listen(serv_sock, 5) == -1) // 연결요청 대기큐를 생성하고, 클라이언트의 연결요청을 기다림
		error_handling("listen() error");

	while (1) // 값 받아오기
	{
		clnt_adr_sz = sizeof(clnt_adr); // 크기 할당
		clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz); // 클라이언트의 연결요청을 수락하고, 클라이언트와의 송수신을 위한 새로운 소켓 생성

		pthread_mutex_lock(&mutx); // 쓰레드 잡기
		clnt_socks[clnt_cnt++] = clnt_sock; // 클라이언트 수와 파일 디스크립터를 등록
		pthread_mutex_unlock(&mutx); // 쓰레드 놓아줌

		pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock); // 쓰레드 생성, handle_clnt함수 실행
		pthread_detach(t_id); // 쓰레드가 종료되면 쓰레드 소멸
		printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr)); // 클라이언트의 IP정보를 문자열로 변환하여 출력
	}
	close(serv_sock);
	return 0;
}

void* handle_clnt(void* arg) // 클라이언트와의 연결을 위해 생성된 소켓의 파일 디스크립터
{
	int clnt_sock = *((int*)arg); // 클라이언트 소켓 할당
	int str_len = 0, i; // 문자열 길이 선언
	char msg[BUF_SIZE]; // 메시지 크기 설정(100)

	while ((str_len = read(clnt_sock, msg, sizeof(msg))) != 0) // 클라이언트로 부터 EOF를 수신할 때까지 읽어서
		send_msg(msg, str_len); // send_msg 함수 호출

	pthread_mutex_lock(&mutx); // 먼저 생성된 쓰레드를 보호
	for (i = 0; i < clnt_cnt; i++) // remove disconnected client
	{
		if (clnt_sock == clnt_socks[i]) // 현재 해당하는 파일 디스크립터를 찾으면
		{
			while (i++ < clnt_cnt - 1) // 클라이언트가 연결요청을 했으므로 해당정보를 덮어씌워 삭제
				clnt_socks[i] = clnt_socks[i + 1];
			break;
		}
	}
	clnt_cnt--; // 클라이언트 수 감소
	pthread_mutex_unlock(&mutx); // 쓰레드를 보호에서 해제
	close(clnt_sock); // 클라이언트와의 송수신을 위한 생성했던 소켓종료
	return NULL;
}
void send_msg(char* msg, int len) // send to all // mutex로 잡고 있는 모든 내용을 보냄
{
	int i;
	pthread_mutex_lock(&mutx); // 뮤텍스 lock
	for (i = 0; i < clnt_cnt; i++) // 현재 연결된 모든 클라이언트에게 메시지를 전송
		write(clnt_socks[i], msg, len);
	pthread_mutex_unlock(&mutx); // 뮤텍스 unlock
}

void error_handling(char* msg) // 에러 핸들링 코드
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1); // -1
}

```

## chat_clint.c

<pre><code>             

#include <stdio.h> // 입출력
#include <stdlib.h> // 문자열 변환, 의사 난수 생성
#include <unistd.h> // 표준 심볼 상수 및 자료형
#include <string.h> // 문자열 상수
#include <arpa/inet.h> // 주소변환
#include <sys/socket.h> // 소켓 연결
#include <pthread.h> // 쓰레드 사용

#define BUF_SIZE 100 // 버퍼 사이즈
#define NAME_SIZE 20 // 사용자 이름

void* send_msg(void* arg); // 메시지를 보냄
void* recv_msg(void* arg); // 메시지를 받음
void error_handling(char* msg); // 에러 핸들링

char name[NAME_SIZE] = "[DEFAULT]"; // 이름 선언 및 초기화
char msg[BUF_SIZE]; // 메시지 선언

int main(int argc, char* argv[])
{
	int sock; // 소켓 선언
	struct sockaddr_in serv_addr; // 소켓 주소 선언
	pthread_t snd_thread, rcv_thread; // 보내고 받는 쓰레드 선언
	void* thread_return; // ip, port, 이름 받기
	if (argc != 4) { // 실행파일 경로/IP/PORT번호/채팅닉네임을 입력으로 받아야 함
		printf("Usage : %s <IP> <port> <name>\n", argv[0]);
		exit(1);
	}

	sprintf(name, "[%s]", argv[3]); // 이름 입력 받음
	sock = socket(PF_INET, SOCK_STREAM, 0); // TCP 소켓 생성

	memset(&serv_addr, 0, sizeof(serv_addr)); // 서버 주소정보 초기화
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

	if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) // 서버 주소정보를 기반으로 연결요청
		error_handling("connect() error!");

	pthread_create(&snd_thread, NULL, send_msg, (void*)&sock); // 쓰레드 생성 및 실행
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock); // 쓰레드 생성 및 실행
	pthread_join(snd_thread, &thread_return); // 쓰레드 종료까지 대기
	pthread_join(rcv_thread, &thread_return); // 쓰레드 종료까지 대기
	close(sock); // 클라이언트 소켓 연결종료
	return 0;
}

void* send_msg(void* arg) // send thread main
{
	int sock = *((int*)arg); // 클라이언트의 파일 디스크립터
	char name_msg[NAME_SIZE + BUF_SIZE];
	while (1)
	{
		fgets(msg, BUF_SIZE, stdin);
		if (!strcmp(msg, "q\n") || !strcmp(msg, "Q\n"))
		{
			close(sock); // 클라이언트 소켓 연결종료 후
			exit(0); // 프로그램 종료
		}
		sprintf(name_msg, "%s %s", name, msg); // client 이름과 msg를 합침
		write(sock, name_msg, strlen(name_msg)); // 널문자 제외하고 서버로 문자열을 보냄
	}
	return NULL;
}

void* recv_msg(void* arg) // read thread main
{
	int sock = *((int*)arg); // 클라이언트의 파일 디스크립터
	char name_msg[NAME_SIZE + BUF_SIZE];
	int str_len;
	while (1)
	{
		str_len = read(sock, name_msg, NAME_SIZE + BUF_SIZE - 1);
		if (str_len == -1) // read 실패시
			return (void*)-1;
		name_msg[str_len] = 0;
		fputs(name_msg, stdout);
	}
	return NULL;
}

void error_handling(char* msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}
</code></pre>
