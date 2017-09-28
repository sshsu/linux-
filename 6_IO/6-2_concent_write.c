/*
	这是一个web服务器上集中写的例子
	http应答包括一个状态行，多个头部字段，一个空行和文本内容，前三部分通常可能被web服务器放在一块内存中，
	而文档的内容通常被读入另一个单独的内存中，不需要讲两个内容拼接在一块进行打包发送，可以使用writev()函数将他们同时写出


	函数原型 
	#include<sys/uio.h>
	ssize_t readv( int fd, const struct iovec* vector, int count );
	ssize_t writev( int fd, const struct iovec* vector, int count );

	fd:要进行操作（read/write）的文件描述符
	
	vector:指向的多块独立内存地址

	count: 一共有多少块内存地址

	因为操作的是内存地址向量所以名称为 writev(write vector)

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
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>

#define BUFFER_SIZE 1024

static const char* status_line[2] = { "200 OK", "500 Internal server error" };

void concen_write(const int & connfd, const char * file_name){

		//http应答的状态行，头部字段和一个空行的缓存区
		char header_buf[ BUFFER_SIZE ] ;
		memset( header_buf, 0 ,sizeof( header_buf ) );
		int len = 0;
		//文件的内容以及属性
		char * file_buf;
		struct stat file_stat;
		//文件是否有效
		bool valid =true;
		int ret;
		if( stat( file_name, &file_stat ) < 0 ){ //目标文件不存在
			valid = false;
		}
		else{
			if( S_ISDIR( file_stat.st_mode ) ){ //目标是一个目录
				valid = false;
			}
			else if ( file_stat.st_mode & S_IROTH ){//当前用户拥有读取目标文件的权限

				//打开文件
				int fd = open ( file_name, O_RDONLY );
				//读取文件
				file_buf = new char [ file_stat.st_size + 1 ];
				memset( file_buf, 0 , sizeof(char)  * (file_stat.st_size + 1) );
				if( read( fd, file_buf, file_stat.st_size ) < 0 ){
					valid = true;
				}
			} 
			else{
				valid = false;
			}

		}

		if( valid ){ //读取文件内容成功
			
			//添加状态行，头字段
			ret = snprintf( header_buf, BUFFER_SIZE-1, "%s %s \r\n", "HTTP/1.1", status_line[0] );
			len += ret;

			//头字段
			ret = snprintf( header_buf + len, BUFFER_SIZE - 1 - len, "Content-Length: %d\r\n", int(file_stat.st_size) );
			len += ret;

			//空行
			ret = snprintf( header_buf + len, BUFFER_SIZE - 1 -len, "%s", "\r\n" );
			len +=ret;

			//使用writev进行集中写
			struct iovec iv[2];
			iv[ 0 ].iov_base = header_buf;
			iv[ 0 ].iov_len = strlen ( header_buf );
			iv[ 1 ].iov_base = file_buf;
			iv[ 1 ].iov_len = file_stat.st_size;
			ret = writev( connfd, iv, 2);
		}
		else{//目标文件无效，通知客户端发生内部错误

			ret = snprintf( header_buf, BUFFER_SIZE - 1, "%s %s \r\n", "HTTP/1.1", status_line[1] );
			len += ret;
			ret = snprintf( header_buf + len, BUFFER_SIZE - 1 - len, "%s", "\r\n" );
			len += ret;
			send( connfd, header_buf, strlen( header_buf), 0 ); 
		}
		close( connfd );
		delete [] file_buf;
}

int main(int argc, char *argv[]){

	if( argc < 4 ){
		printf( "usage: %s ip_number, port_number, filename\n", basename( argv[0] ));
		return 1;
	}

	//获取参数
	const char *ip = argv[1];
	int port = atoi ( argv[2] );
	const char * file_name = argv[3];

	//创建sock地址
	struct sockaddr_in server_addr;
	bzero( &server_addr, sizeof( server_addr ));
	server_addr.sin_family = AF_INET;
	inet_pton( AF_INET, ip, &server_addr.sin_addr );
	server_addr.sin_port = htons( port );

	//创建socket
	int sockfd = socket( AF_INET, SOCK_STREAM, 0 );
	assert( sockfd > 0 );

	//命名绑定socket
	int ret = bind( sockfd, (struct sockaddr * )& server_addr, sizeof( server_addr ) );
	assert( ret != -1 );

	//开启监听队列
	ret = listen ( sockfd, 5 );
	assert( ret != -1 );

	//接受客户端的请求
	struct sockaddr_in client;
	socklen_t client_addr_length = sizeof( client ); 
	int connfd = accept( sockfd, ( struct sockaddr * )&server_addr, &client_addr_length );
	if( connfd < 0 ){
		printf(" errno is: %d\n", errno);
	}
	else{

		concen_write( connfd, file_name );
	}

	close( sockfd );
	return 0;
}