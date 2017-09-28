/*
这是一个访问远端daytime的服务
通过gethostname获取服务器ip
通过gerservbyname获取端口
建立socket从缓存区中读取信息

输入参数为host的名称，查找该host对应的ip的时候会首先从/etc/host中获取信息，如果获取不到就使用DNS服务进行获取
*/

#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<stdio.h>
#include<unistd.h>
#include<assert.h>
#include<string.h>

int main( int argc, char *argv[] ){

	if(argc!=2){
		printf("usage: %s hostname\n",basename( argv[0]) );
		return 1;
	}

	char *host = argv[1];

	//获取目标主机的ip地址
	struct hostent* hostinfo = gethostbyname( host );
	assert(hostinfo);
	//获取daytime的服务信息
	struct servent* servinfo = getservbyname( "daytime", "tcp");
	assert( servinfo );
	printf("serv daytime port is %d\n", ntohs( servinfo->s_port ));
	//创建socket地址
	struct sockaddr_in address;
	bzero( &address, sizeof( address ) );
	address.sin_family = AF_INET;
	address.sin_port = servinfo->s_port;
	address.sin_addr = * ( (struct in_addr*) *(hostinfo->h_addr_list) ) ;
	//获取socket描述符
	int sockfd = socket( AF_INET, SOCK_STREAM, 0 );
	int result = connect( sockfd, ( struct sockaddr* ) &address, sizeof( address) );
	assert( result != -1);

	//获取内容
	char buffer[128];
	result = read( sockfd, buffer, sizeof(buffer));
	assert( result > 0 );
	buffer[ result ] = '\0';
	printf( "the day time is %s\n", buffer );
	close( sockfd ); 	
	return 0;
}