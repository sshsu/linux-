/*
	这是一个发送文件的程序
	
	函数原型
	#include<sys/sendfile.h>
	ssize_t sendfile( int out_fd, int in_fd, off_t* offset, size_t count)

	out_fd: 待写入数据的文件描述符，必须是一个socket
	in_fd: 指向一个真实的文件的描述符，不能是socket或管道
	   |----offset:从in_fd那个位置开始读
	   |----count:从in_fd传了多少到out_fd中	  
*/

#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<sys/sendfile.h>

int main( int argc, char *argv[] ){

	if( argc < 4 ){
		printf("usage: %s ip_number port_number filename\n",basename( argv[0] ));
		return 1;
	}

	//获取参数
	const char* ip = argv[1];
	int port = atoi ( argv[2] );
	const char* file_name = argv[3];

	//打开文件，并获取属性
	int filefd = open ( file_name, O_RDONLY );
	assert( filefd > 0 );
	struct stat stat_buf;
	fstat( filefd, &stat_buf );

	//创建socker地址
	struct sockaddr_in server_addr;
	bzero( &server_addr, sizeof( server_addr ) );
	server_addr.sin_family = AF_INET;
	inet_pton( AF_INET, ip, &server_addr.sin_addr );
	server_addr.sin_port = htons( port );

	//获取socket描述符
	int sockfd = socket( AF_INET, SOCK_STREAM, 0);
	assert( sockfd > 0 );

	//绑定重命名socket
	int ret = bind( sockfd, ( struct sockaddr*)&server_addr, sizeof( server_addr ) );
	assert( ret != -1 );

	//监听
	ret = listen ( sockfd, 5 );
	assert( ret != -1 );

	//连接客户端
	struct sockaddr_in client;
	socklen_t client_addr_length = sizeof( client );
	int connfd = accept ( sockfd, ( struct sockaddr* )&server_addr, &client_addr_length );
	if( connfd < 0 ){
		printf( "errno is %d \n", errno);

	}
	else{

		sendfile( connfd, filefd, 0, stat_buf.st_size );
		close( connfd );
	}
	close( sockfd );

	return 0;
}