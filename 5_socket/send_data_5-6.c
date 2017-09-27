/*
	发送带外数据客户端与rec_data5-7发送带外数据服务端一起
	发送函数

	ssize_t send(int sockfd, void *buf, size_t len,int flags);
	send读取sockfd上的数据，buf和len参数分别指定缓冲区和大小,flags通常设置为0

	ssize_t recv(int sockfd, void *buf,size_t len, int flags);
	recv读取sockfd上的数据，buf和len参数分别制定缓冲区和大小，flags通常设置为0，是一些选项名

	最终效果和书本上实验有所不同，带外数据没有发送成功,不知道哪里出错了。

*/

#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>

typedef struct sockaddr_in sockaddr_v4;

int main(int argc, char *argv[]) {
	if (argc < 3) {
		printf("usage: %s ip_address port_number\n", basename(argv[0]));
		return 1;
	}

	const char *ip = argv[1];
	const int port = atoi(argv[2]);

	//创建sockaddr_in
	sockaddr_v4 server_address;
	bzero( &server_address, sizeof( server_address ) );
	server_address.sin_family = AF_INET;
	inet_pton( AF_INET, ip, &server_address.sin_addr );
	server_address.sin_port =  htons( port );

	//获取socket描述符
	int sockfd = socket( PF_INET, SOCK_STREAM, 0 );
	assert( sockfd >= 0 );
	if ( connect( sockfd, ( struct sockaddr* )&server_address, sizeof( server_address ) ) < 0 ){
		printf( "connection failed\n" );
	}
	else{
		const char *oob_data = "abc";
		const char *normal_data = "123";
		send( sockfd, normal_data, strlen( normal_data ), 0 );
		send( sockfd, oob_data, strlen( oob_data ), MSG_OOB );
		send( sockfd, normal_data, strlen( normal_data ), 0 );
	}
	close( sockfd );
		return 0;
}