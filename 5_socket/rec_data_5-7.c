#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<errno.h>

#define BUF_SIZE 1024
typedef struct sockaddr_in sockaddr_v4;

int main(int argc ,char *argv[]){
	if(argc < 3){
		printf("usage: %s ip_address port_number\n",basename(argv[0]));
		return 1;
	}

	const char* ip = argv[1];
	int port = atoi(argv[2]);

	sockaddr_v4 server_address;
	bzero(&server_address, sizeof( server_address ) );
	server_address.sin_family = AF_INET;
	inet_pton( AF_INET, ip, &server_address.sin_addr );
	server_address.sin_port = htons( port ); 

	int sockfd = socket( PF_INET, SOCK_STREAM, 0 );
	assert( sockfd > 0 );

	int ret = bind( sockfd, ( struct sockaddr* )&server_address, sizeof( server_address ) );
	assert( ret != -1 );

	ret = listen( sockfd, 5 );
	assert( ret != -1 );

	sockaddr_v4 client;
	socklen_t client_addr_length = sizeof( client );
	int connfd = accept( sockfd, ( struct sockaddr* )&client, &client_addr_length );
	if( connfd < 0 ){
		printf( "errno is %d\n", errno );
	}
	else{
		char buf[ BUF_SIZE ];
		memset( buf, 0 , sizeof( buf ));
		ret = recv( connfd, buf, BUF_SIZE-1, 0 );
		printf( "got %d bytes of normal data %s\n", ret, buf);

		memset( buf, 0, sizeof( buf ) );
		ret = recv( connfd, buf, BUF_SIZE-1, MSG_OOB );
		printf( "got %d bytes of oob data %s\n", ret, buf);

		memset( buf, 0, sizeof( buf ) );
		ret = recv( connfd, buf, BUF_SIZE-1, 0 );
		printf( "got %d bytes of normal data %s\n", ret, buf);

			/*	memset( buf, 0, sizeof( buf ) );
		ret = recv( connfd, buf, BUF_SIZE-1, 0 );
		printf( "got %d bytes of normal data %s\n", ret, buf);*/
		close( connfd );
	}

	close( sockfd );
	return 0;
}