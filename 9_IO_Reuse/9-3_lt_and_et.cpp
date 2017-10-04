/*
此程序是为了比较epoll的LT(水平触发)和ET(边缘触发)，将只读缓冲区设成10的大小，使用telnet发送超过10字节的数据过去分析两种的处理方式

Level_triggered(水平触发)：当被监控的文件描述符上有可读写事件发生时，epoll_wait()会通知处理程序去读写。
如果这次没有把数据一次性全部读写完(如读写缓冲区太小)，那么下次调用 epoll_wait()时，它还会通知你在上没读写完的文件描述符上继续读写，
当然如果你一直不去读写，它会一直通知你！如果系统中有大量你不需要读写的就绪文件描述符，而它们每次都会返回，
这样会大大降低处理程序检索自己关心的就绪文件描述符的效率！

Edge_triggered(边缘触发)：当被监控的文件描述符上有可读写事件发生时，epoll_wait()会通知处理程序去读写。
如果这次没有把数据全部读写完(如读写缓冲区太小)，那么下次调用epoll_wait()时，它不会通知你，也就是它只会通知你一次，
直到该文件描述符上出现第二次可读写事件才会通知你！！！这种模式比水平触发效率高，系统不会充斥大量你不关心的就绪文件描述符

epoll是用来监听文件描述符对应的事件的系统,详见:http://www.cnblogs.com/charlesblc/p/6242479.html
一般的处理过程为：
 使用epoll_create()创建一个epoll对象
 使用epoll_ctl()添加/删除监听事件
 使用epoll_wait()获得触发的事件

#include<sys/epoll.h>
 int epoll_ctl( int epfd, int op, int fd, struct epoll_event *event);

向epoll对象epfd进行op类型操作(增加/删除)fd对应的时间event;

struct epoll_event{
    _uint32_t events;//epoll事件
    epoll_data_t data;//用户数据,是一个联合体，一般而言是fd的值
 }

epoll_event中的data并不是多余的，虽然epoll_ctl()可以从fd获取值，有点冗余的感觉，但是后续的epoll_wait()可以通过该结构体获取事件和
对应的fd

水平触发

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
            //向epoll对象中添加一个监听事件
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
            //向epoll对象中添加一个监听事件
            addfd( epollfd, connfd, true);
        }
        else if( events[i].events & EPOLLIN ){
            printf( "event trigger once\n ");
            while(1){
                memset( buf, 0, sizeof( buf ) );
                int ret = recv ( sockfd, buf, BUFFER_SIZE -1 , 0 );
                if( ret < 0   ){
                	//数据读取完毕了 
                    if(( errno == EAGAIN || errno == EWOULDBLOCK)){
                        printf(" read later\n");
                        break;
                    }
                    close( sockfd );
                    break;
                }
                //接收数据时网络中断了，返回0。
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