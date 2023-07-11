#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024 // 메시지 버퍼의 크기 정의

int main(int argc, char *argv[])
{
	int sock;
	char message[BUF_SIZE];
	int str_len;
	struct sockaddr_in serv_addr;

    // 서버의 IP와 Port
	if(argc != 3){
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}

    // 소켓 생성
	sock = socket(PF_INET, SOCK_STREAM, 0);
	if(sock == -1)
		perror("socket() error");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

    // IP와 port를 통해 서버 주소 초기화
	if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		perror("connect() error!");
	else
		puts("Connected..");

    // 연결 성공 시 loop
	while(1)
	{
		fputs("Input message(Q to quit) : ", stdout); // 채팅 입력
		fgets(message, BUF_SIZE, stdin);

        // q 또는 Q를 입력하면 채팅 종료 (loop 종료)
		if(!strcmp(message, "q\n") || !strcmp(message, "Q\n"))
			break;

		write(sock, message, strlen(message)); // 서버에 메시지 전송
		str_len = read(sock, message, BUF_SIZE-1); // 수신 응답
		
        // 수신된 메시지가 null로 종료되고 콘솔에 출력됨
        message[str_len] = 0;
		printf("Message from server : %s", message);
	} 
	close(sock); // 클라이언트 소켓 닫기
	return 0;
}
