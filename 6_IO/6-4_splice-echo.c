/*
	这是一个回射客户端发送内容的的程序
	
	函数原型
	#include<fcntl.h>
	ssize_t splice( int fd_in, loff_t* off_in, int fd_out, loff_t* off_out, size_t len,
					 unsigned int flags);
	fd_in:输入数据文件描述符
		|---off_in: 如果fd_in是一个管道文件，那么off_in设为NULL, 否则表示从输入数据流何处开始读数据
	fd_out:输出文件描述符
		|---off_out: 如果fd_out是一个管道文件，那么off_out设为NULL,否则是数据写入的偏移
	len:指定移动数据的长度
	flags: 控制数据如何移动的,多个值一起使用的时候，用按位或进行
		|== SPLICE_F_MOVE: 给内核一个提示，合适的话，整页移动数据，自2.6.21后，没有效果了
		|== SPLICE_F_NONBLOCK: 非阻塞的splice操作，但实际效果还会收文件描述符本身的阻塞状态影响
		|== SPLICE_F_MORE: 给内核一个提示，后续的splice调用将读取更多数据
		|== SPLICE_F_GIFT: 无效果

	使用splice函数时，fd_in和fd_out必须至少有一个是管道文件描述符。splice函数调用成功时返回移动字节的数量。
	失败的时候返回-1,并设置errno
	errno
		|== EBADF: 参数所指描述符有错
		|== EINVAL:系统不支持splice,或文件以追加方式打开，或两个文件描述符都不是管道文件描述符，或offset被用于
		不支持随机访问的设备。
	ENOMEM:内存不够
	ESPIPE:参数fd_in(fd_out)是管道文件描述符，而off_in(或off_out)不为NULL.
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

	if( argc < 3 ){
		printf("usage: %s ip_number port_number filename\n",basename( argv[0] ));
		return 1;
	}

	//获取参数
	const char* ip = argv[1];
	int port = atoi ( argv[2] );
	const char* file_name = argv[3];


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

		int pipefd[2];
		assert( ret != -1 );
		//创建管道
		ret = pipe( pipefd );
		//将socket中的数据定向到管道写端fd[1]中去
		ret = splice( connfd, NULL, pipefd[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE );
		assert( ret != -1 );
		//将管道文件读端fd[0]的数据重定向到socket中去
		ret = splice( pipefd[0], NULL, connfd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MORE );
		assert( ret!= -1 );
		close( connfd );
	}
	close( sockfd );
	return 0;
}