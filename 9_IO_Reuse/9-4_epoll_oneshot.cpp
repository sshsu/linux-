/*
此程序为epoll事件的EPOLLONESHOT事件示例
EPOLLONESHOT实现通过为文件描述符设置标志位，该文件描述符监听的事件触发之后，标志位置假，然后该fd要再次触发事件的时候因为检查到标志为假说明正在对该文件进行操作，当前无法操作，

*/


#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<fcntl.h>
#include<stdio.h>
#include<sys/epoll.h>
#include<pthread.h>
#include <stdlib.h>


#define MAX_EVENT_NUMBER 1024
#define BUFFER_SIZE 10

int set_no_blocking(int fd ){
    int old_option = fcntl( fd, F_GETFL );
    int new_option = old_option | O_NONBLOCK;
    fcntl( fd, F_SETFL, new_option );
    return old_option;
};

void addfd( int epollfd, int fd, bool enable_et){
    //创建事件对象，并初始化
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN;
    //默认为lt模式，开启et模式
    if( enable_et ){
        event.events |= EPOLLET;
    }
    epoll_ctl( epollfd, EPOLL_CTL_ADD, fd, &event );
    set_no_blocking( fd );
}

void lt( epoll_event* events, int number , int epollfd, int listenfd){
    char buf[ BUFFER_SIZE ];
    for( int i=0 ; i< number ; i++ ){
        int sockfd = events[i].data.fd;
        if( sockfd == listenfd){
            struct sockaddr_in client_address;
            socklen_t client_address_len = sizeof( client_address );
            int connfd = accept(sockfd, (struct sockaddr*)&client_address, &client_address_len);
            if( connfd < 0){
                continue;
            }
            addfd( epollfd, connfd, false);
        }
        else if ( events[i].events & EPOLLIN){
            printf("event trigger once\n");
            memset( buf, 0, sizeof( buf ) );
            int ret = recv( sockfd, buf, BUFFER_SIZE-1, 0);
            if( ret <= 0 ){
                close( sockfd );
                continue;
            }
            printf( "get %d bytes of content: %s\n",ret, buf);
        }
        else{
            printf("something else happened \n");
        }

    }

}

void et( epoll_event *events, int number, int epollfd, int listenfd){
    char buf[ BUFFER_SIZE ];
    for( int i = 0 ; i < number ; i++ ){
        int sockfd = events[i].data.fd;
        if( sockfd == listenfd ){
            struct sockaddr_in client_address;
            socklen_t client_address_len = sizeof( client_address );
            int connfd = accept( sockfd, (struct sockaddr*)&client_address, &client_address_len );
            if( connfd < 0){
                continue;
            }
            addfd( epollfd, connfd, true);
        }
        else if( events[i].events & EPOLLIN ){
            printf( "event trigger once\n ");
            while(1){
                memset( buf, 0, sizeof( buf ) );
                int ret = recv ( sockfd, buf, BUFFER_SIZE -1 , 0 );
                if( ret < 0   ){
                    if(( errno == EAGAIN || errno == EWOULDBLOCK)){
                        printf(" read later\n");
                        break;
                    }
                    close( sockfd );
                    break;
                }
                else if( ret == 0 ){
                    close( sockfd );
                }
                else{
                    printf("get %d bytes of content: %s\n", ret, buf );
                }

            }

        }
        else {
            printf( "something else happened \n");
        }

    }
}

int main(int argc, char *argv[]){

    if( argc < 3 ){
        printf("usage : %s ip_number port number", basename( argv[0] ));
        return -1;
    }
    const char *ip = argv[1];
    int port = atoi( argv[2] );

    struct sockaddr_in server_address;
    bzero( &server_address, sizeof( server_address ));
    server_address.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &server_address.sin_addr);
    server_address.sin_port = htons( port );

    int sockfd = socket(AF_INET, SOCK_STREAM, 0 );
    assert( sockfd >= 0 );
    int ret = bind (sockfd, (struct sockaddr*)&server_address, sizeof(server_address) );
    assert( ret != -1 );
    ret = listen( sockfd, 5);
    assert( ret != -1 );

    //创建事件对象数组
    epoll_event events[ MAX_EVENT_NUMBER ];
    //创建epoll对象
    int epollfd = epoll_create( 5 );
    assert( epollfd != -1 );
    addfd( epollfd, sockfd, true );

    while(1){
        //获取触发的监听事件
        ret = epoll_wait( epollfd, events, MAX_EVENT_NUMBER, -1 );

        if( ret < 0 ){
            printf( "epoll failure\n");
        }

//        lt( events, ret, epollfd, sockfd );
        et( events, ret, epollfd, sockfd );

    }
    return 0;
}
