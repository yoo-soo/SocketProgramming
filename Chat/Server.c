#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <pthread.h>
#include <signal.h>

// 각 클라이언트 연결에 대한 정보를 저장
// 파일 디스크럽터(file descriptor)와 IP 주소(ip address)를 포함
struct thread_data{ 
    int fd; // 파일 디스크럽터 값
    char ip[20]; // IP 주소 값
};

// 각 thread에서 실행되는 메인 루틴
// thread_data 구조체에 대한 포인터를 인수로 수신하여 클라이언트와 통신
void *ThreadMain(void *argument);

// get sockaddr(ipaddr), IPv4 or IPv6
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) { // IPv4
        return &((( struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr); // IPv6
}

int main(void)
{
    int sockfd, new_fd;
    struct sockaddr_storage client_addr;
    struct sockaddr_in servaddr;
    socklen_t sin_size;
    char address[20];
    int ret;
    int opt = 1;

    pthread_t thread_id;

    signal( SIGPIPE, SIG_IGN );

    // 소켓 생성 함수
    sockfd = socket(AF_INET, SOCK_STREAM,0);

    // 소켓 옵션 설정 (addr, PORT 재사용)
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)); 

    // 서버의 주소 값을 담을 변수 초기화
    // 임의로 domain, port number 지정
    bzero( &servaddr, sizeof(servaddr)); 
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    unsigned int port = 9897;
    servaddr.sin_port = htons(port);

    // 소켓 생성 후, 소켓의 address와 port number를 묶어주는 역할
    ret = bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if ( ret != 0 )
    {
        perror("bind");
        exit(-1);
    }

    // 수신 클라이언트 연결 수신
    // backlog는 소켓에 연결하기 위해 들어가있는 큐의 크기
    ret = listen(sockfd, 5); //6
    if ( ret != 0 )
    {
        perror("listen");
        exit(-1);
    }

    while(1) {  // main accept() loop

        printf( "채팅 클라이언트 대기 중\n" );

        sin_size = sizeof client_addr;

        // 클라이언트 소켓이 연결되면 해당 클라이언트 소켓의 파일디스크립터를 반환하는 함수
        new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
        
        //새로운 클라이언트가 접속 할 때까지 기다린다.
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        // 클라이언트 IP 주소 추출
        inet_ntop(client_addr.ss_family,
            get_in_addr((struct sockaddr *)&client_addr),
            address, sizeof address);
        
        client_cnt += 1;
        printf("IP(%s) 접속하였습니다.\n", address);
    
        // 메모리가 할당되는 구조체, 클라이언트는 thread_data에 저장됨.
        struct thread_data *data;
        data= (struct thread_data*)malloc(sizeof(struct thread_data));
        data->fd = new_fd;
        strcpy( data->ip,address);

        // 클라이언트 소켓이 들어오면 새로운 thread를 생성해 하나의 thread와 하나의 클라이언트가 통신하도록 함.
        // ThreadMain 함수가 새로운 스레드에서 실행됨.
        pthread_create( &thread_id, NULL, ThreadMain, (void*)data);
    }

    return 0;
}

/* 스레드로 작동하는 함수.
argument -> 구조체인데 ip, fd들어있다.
*/

void *ThreadMain(void *argument)
{
    struct thread_data *client_data;
//    char buf[1024];
    int buf_len;

    /* pthread_detach(pthread_t th);
    ->pthread_join()을 이용하지 않아도 스레드가 종료될때 자원이 반납된다.
    th -> 스레드 식별자. 성공하면 0, 실패하면 error 리턴
    */

    pthread_detach(pthread_self());
    client_data = (struct thread_data*)argument;
    int fd = client_data->fd;
    char ip[20];
    strcpy( ip, client_data->ip);

    // thread 함수
    // 반복문을 돌며 클라이언트와 통신함
    // 클라이언트가 데이터를 받아오면 콘솔창에 출력하고, 그 데이터 그대로 다시 클라이언트에 수신
    // ECHO 서버
    while(1)
    {
        char buf[1024] = {0};

        // 클라이언트의 데이터를 읽고 콘솔에 출력
        int num = read( fd, buf, sizeof(buf) );

        // 클라이언트가 통신을 끊으면 read함수는 0을 리턴.
        if ( num == 0 ) 
        {
          printf("connection end bye\n"); // 통신이 끊어지면 메시지 출력
          break;
        }

        printf("recv %s : [%s] %dbyte\n",ip, buf, num );
        // 수신된 데이터를 클라이언트에 다시 작성
        write( fd, buf, strlen(buf));
    }

    // 연결이 끊긴 클라이언트 ip 정보 메시지
    printf("disconnected client ip %s\n", ip );
    free(client_data);
    close(client_data->fd);

    return 0;
}
