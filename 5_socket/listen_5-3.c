
/*
this is a port listening program which input args includs ip_address, port and backlog(which means maxinum of tcp establsh)
we can use command 'telnet $ip_address $port'and 'netstat -nt | grep $port ' to test.
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
static bool stop=false;

static void handle_term(int sig){
	stop=true;
}

int main(int argc, char *argv[]){
	signal(SIGTERM,handle_term);//绑定事件处理函数

	if(argc<3){
		printf("uage %s ip_address port_number backlog\n ",
				basename(argv[0]));
		return 1;
	}
	//获取参数
	const char *ip=argv[1];
	int port=atoi(argv[2]);
	int backlog=atoi(argv[3]);
	printf("%s\n",ip);
	printf("%d\n",port);

	//
	int sock=socket(PF_INET, SOCK_STREAM,0);
	assert(sock>=0);

	struct sockaddr_in address;
	bzero( &address, sizeof(address) );
	address.sin_family = AF_INET;
	inet_pton(AF_INET,ip,&address.sin_addr);
	address.sin_port=htons(port);

	int ret=bind(sock, (struct sockaddr * )&address , sizeof(address));
	assert(ret!=-1);

	ret=listen(sock,backlog);
	assert(ret!=-1);

	while(!stop){
		sleep(1);
	}

	close(sock);
	return 0;
}
