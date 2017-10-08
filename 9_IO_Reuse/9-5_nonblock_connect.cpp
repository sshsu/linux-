/*
###程序说明
 此程序是一个非阻塞connect的demo

###实现概要
 对socket进行connect时，连接还没有建立的时，该socket的errno为EINPROGRESS,因此将socket设置成非阻塞的进行connect时，可使用select(设置timeout)去监听socket的写事件（连接建立之后会触发socket的写事件）来获得连接

###函数介绍
 getsockopt( sockfd, SOL_SOCKET, SO_ERROR, &error, &lengeth)
 获取sockfd上的SOL_SOCKET层山的SO_ERROR值，并返回到error中，长度为length,成功为0，失败-1

 select( sockfd +1, NULL, writefds, NULL, &timeout );
 监听writefds集合中属于[0,sockfd+1）区间的写事件，超时事件为timeout,结果返回到writefds, 超时返回0，失败返回-1，被信号所终端设置为EINTR,成功返回就绪的文件描述符的总数
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <poll.h>

#define BUFFER_SIZE 1023

int setnonblocking( int fd ){
    //获取fd的旧选项
    int old_option = fcntl( fd, F_GETFL );
    //添加非阻塞
    int new_option = old_option | O_NONBLOCK;
    //设置新选项
    fcntl( fd, F_SETFL, new_option);
    return old_option;
}

/*这是记性阻塞connect的主要函数，先把sockfd设置成非阻塞,然后利用sockfd有连接将会触发写事件，从而使用select进行监听
 *但监听处理的很细致，select结束有可能是因为超时，或者其余的错误，就算监听到sockfd本身已经进行连接了还需要获取errno
 *去查看当前的socket是否有错
 */

int unblock_connect( const char* ip, int port, int time ){

    struct sockaddr_in addr;
    bzero( &addr, sizeof( addr ));
    addr.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &addr.sin_addr);
    addr.sin_port = htons( port );

    int sockfd = socket( AF_INET, SOCK_STREAM, 0);
    if( sockfd < 0 ){
        return -1;
    }

    //设置非阻塞socket
    int fdopt = setnonblocking( sockfd );
    int ret = connect( sockfd, ( struct sockaddr* )&addr, sizeof( addr ));
    if( ret == 0 ){//如果连接成功
        printf(" connct with server immediately \n");
        fcntl( sockfd, F_SETFL, fdopt );//恢复阻塞属性
        return sockfd;
    }
    else if( errno != EINPROGRESS ){//连接还未成功，并且不处于连接状态
        printf("unblock connect not support\n ");
        close( sockfd );
        return -1;
    }

    fd_set writefds;
    struct timeval timeout;

    FD_ZERO( &writefds );
    FD_SET( sockfd, &writefds);

    timeout.tv_sec = time;
    timeout.tv_usec = 0;

    ret = select( sockfd+1, NULL, &writefds, NULL, &timeout );
    if( ret <= 0 ) {//select超时或出错
        printf("connection time out\n");
        close( sockfd );
        return -1;
    }

    if( !FD_ISSET( sockfd, &writefds )){//指定连接的sockfd没有发生连接
        printf("no events on sockfd found\n");
        close( sockfd );
        return -1;
    }

    int error = 0;
    socklen_t length = sizeof( error );
    //获取sockfd上的错误，检查连接是否正常
    if( getsockopt( sockfd, SOL_SOCKET, SO_ERROR, &error, &length) < 0 ){
        printf("get socket option failed\n");
        close( sockfd );
        return -1;

    }

    //如果连接出现了问题
    if( error != 0 ){
        printf(" get socket option failed\n");
        close( sockfd );
        return -1;
    }

    //连接成功,设置回sockfd为阻塞
    printf("connection ready after select with the socket:%d\n", sockfd);
    fcntl( sockfd, F_SETFL, fdopt );
    return sockfd;




}


int main(int argc, char* argv[])
{

    if( argc < 2 ){
        printf("usage：%s ip_number port_number\n", basename( argv[0] ));
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi( argv[2]);

    int sockfd = unblock_connect( ip, port, 10);
    if( sockfd < 0 ){
        return 1;
    }
    close( sockfd );
    return 0;
}
