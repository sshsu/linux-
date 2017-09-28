/*
	linux中的进程使用文件描述符的IO概念，对于每一个进程都有一个文件描述数组表，对应的内容为相应的文件信息对象（地址，文件信息等）
	默认情况下:

	fd			descr
	0			STDIN_FILENO 
    1			STDOUT_FILENO 
    2			STDERR_FILENO 


	此程序使用了close(STDOUT_FILENO)关闭了1所对应的标准输出文件描述符

	使用dup(connfd)复制了一份连接的connfd指向的内容的描述符，并放在了进程文件描述数组表最低可以位置(刚刚关掉了1所以1是最低可用的)

	使用printf()的时候是默认使用进程的1描述符对应的文件，导致了输出被重定向了 


	函数原型
	#include<unistd.h>
	int dup( int file_descritor );
	int dup2( int file_descriptor_1, int file_descriptor_2 );

	dup复制一份file_descriptor对应存放的文件对象地址，放在进程描述符表的最低可用位置
	dup2复制一份file_descriptor_1对应存放的文件对象地址，放在进程描述附表的不小于file_descriptor_2的位置。


*/
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<signal.h>
#include<unistd.h>
#include<stdlib.h>
#include<assert.h>
#include<stdio.h>
#include<string.h>
#include<stdbool.h>
#include<errno.h>


int main( int argc, char *argv[]){

	if( argc < 3 ){
		printf( "usage: %s ip_number port_number\n", basename(argv[0]) );
		return 1;
	}

	//获取参数
	const char *ip = argv[1];
	int port = atoi( argv[2] );

	//创建sock地址
	struct sockaddr_in server_addr;
	bzero( &server_addr, sizeof( server_addr ));
	server_addr.sin_family = AF_INET;
	inet_pton( AF_INET, ip, &server_addr.sin_addr );
	server_addr.sin_port = htons( port );

	//创建socket
	int sockfd = socket( AF_INET, SOCK_STREAM, 0 );
	assert( socket > 0 );

	//命名socket
	int ret = bind( sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr) );
	assert( ret != -1 );

	//监听
	ret = listen( sockfd, 5 );
	assert( ret != -1 );

	//接收
	struct sockaddr_in client;
	socklen_t client_addr_length = sizeof( client );
	int connfd = accept( sockfd, (struct sockaddr*) &client, &client_addr_length);
	if( connfd < 0 ){
		printf(" errono is: %d\n", errno);
	}
	else{
		//关闭fd[1]并将connfd的内容复制到fd[1]中
		close( STDOUT_FILENO );
		dup( connfd );
		printf(" hello\n ");
		close( connfd );

	}

	close( sockfd );
	return 0;
}