/*
程序5-10与5-11作为修改TCP缓冲区的实例
此为发送端

```code

#include<sys/socket.h>
int getsockopt( int sockfd, int level ,int option_name, void * option_value,socklen_t* restrict option_len);
int setsockopt( int sockfd, int level ,int option_name, void * option_value,socklen_t option_len);

sockfd: socket文件描述符
level: 指定的选择的所属层次，分别有SOL_SOCKET（通用socket选项）,IPPROTO_IP(ipv4),IPPROTO_IPV6, IPPROTO_TCp(tcp选项)
option_name: 要修改的选项,
option_value: 要修改的值
option_len: option_value的长度(sizeof()); 


```code
*/

#include<sys/socket.h>
#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<assert.h>
#include<errno.h>

typedef struct sockaddr_in sockaddr_v4;

int main( int argc, char *argv[] ){
	if(argc<3){
		printf( "usageL %s ip_address port_number send_buffer_size\n",basename( argv[0] ) );
		return 1;
	}

	const char *ip = argv[1];
	const int port = atoi( argv[2] );
	int send_buffer_size = atoi( argv[3] );
	int len = sizeof( send_buffer_size );

	sockaddr_v4 server_address;
	bzero( &server_address, sizeof( server_address ) );
	server_address.sin_family=AF_INET;
	inet_pton( AF_INET, ip, &server_address.sin_addr );
	server_address.sin_port = htons( port );

	int sock = socket( AF_INET, SOCK_STREAM, 0);
	assert( sock > 0 );

	setsockopt( sock, SOL_SOCKET, SO_SNDBUF, &send_buffer_size, len );
	getsockopt( sock, SOL_SOCKET, SO_SNDBUF, &send_buffer_size, ( socklen_t* )&len );
	printf( "the tcp send buffer size after setting is %d\n", send_buffer_size );

	if( connect( sock, ( struct sockaddr* )&server_address, sizeof( server_address ) )!=-1 ){
		char buffer[ send_buffer_size ];
		memset( buffer, 'a', send_buffer_size );
		send( sock, buffer, send_buffer_size, 0 );
	}
	close( sock );
	return 0;
}