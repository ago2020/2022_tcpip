#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUFSIZE 30

void error_handling(char *message);

int main(int argc, char *argv[])
{
	int clint_sock;
	char message[BUFSIZE];
	int str_len,addr_size,i;
	char msg1[] = "good";
	char msg2[] = "afternoon!";
	char msg3[] = "everybody!";
	struct sockaddr_in serv_addr;
	struct sockaddr_in from_addr;
	int clnt_addr_size;

	clint_sock = socket(PF_INET, SOCK_DGRAM,0);

	if(clint_sock == -1)
	{
		error_handling("sockt() error");
	}
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

	sendto(clint_sock,msg1,strlen(msg1),0,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
	sendto(clint_sock,msg2,strlen(msg2),0,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
	sendto(clint_sock,msg3,strlen(msg3),0,(struct sockaddr*)&serv_addr,sizeof(serv_addr));

	for(i=0; i<3; i++)
	{
		addr_size = sizeof(from_addr);
		str_len = recvfrom(clint_sock,message,BUFSIZE,0,(struct sockaddr*)&from_addr, &addr_size);
		message[str_len] = 0;
		printf("서버로부터 전송된 메세지 : %s \n",message);
	}
	close(clint_sock);

	return 0;
}
void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
