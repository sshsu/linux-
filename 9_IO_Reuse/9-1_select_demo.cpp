/*

 #include<sys/select.h>
 int select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timeval* timeout)

  函数说明:
  此函数搜索[0, nfds）的文件描述符，检查是否有发生I/O事件(读，写，异常)，并将结果返回在readfds，writefds，writefds集合中

  参数说明:
  nfds:指定监听的文件描述符的区间为[0, nfds）
  readfds:文件描述符发生读IO事件的记录集合
  writefds:文件描述符发生写IO事件的记录集合
  exceotfds:文件描述符发生异常事件的记录集合
  timeout:select这个操作的等待时间

  结构说明:
  typedef struct{
    int fds[];
  }fd_set;
  fd_set中有一个数组，数组的每个元素中的每个bit位代表对应的文件描述符

  由于位操作太过于繁琐，可使用给下列宏对fd_set进行操作
  #include<sys/select.h>
  FD_ZERO (fd_set *fdset);              //清除fd_set所有位
  FD_SET (int fd, fd_set *fdset);       //设置fd_set的位fd
  FD_CLR (int fd, fd_set *fdset);       //清除fd_set的位fd
  int FD_ISSET (int fd, fd_set *fdset); //测试fd_set的位fd是否被设置

  一般的使用select的进行I/O监听复用的流程为
  1.设置好监听集合readfds，writefds，exceotfds中的位，说明要位所对应文件描述符的IO事件(读，写，异常）
  2.调用select，内核将对应监听的事件的结果返回在readfds，writefds，exceotfds中
  3.使用FD_ISSET(fd, readfds)去判断文件是否有对应的事件触发



  由于socket接收了带外数据之后，处于异常状态，使用select可以监测到这种状态，从而分别处理普通数据和带外数据，如下程序

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

#define BUF_SIZE 1024

int main(int argc, char* argv[])
{
    if( argc <= 2 )
    {
        printf("Usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);

    int ret = 0;
    struct sockaddr_in address;
    bzero( &address, sizeof( address ));
    address.sin_family = AF_INET;
    inet_pton( AF_INET,  ip, &address.sin_addr );
    address.sin_port = htons ( port );

    int listenfd = socket( AF_INET, SOCK_STREAM, 0 );
    assert( listenfd >= 0 );

    ret = bind( listenfd, (struct sockaddr*)&address, sizeof(address));
    assert( ret != -1 );

    ret = listen( listenfd, 5 );
    assert( ret != -1 );

    struct sockaddr_in client_address;
    socklen_t  client_address_len = sizeof( client_address );
    int connfd = accept( listenfd, ( struct sockaddr* )&client_address, &client_address_len );
    if( connfd < 0){
        printf("\n");
    }
    else{

        char buf[ BUF_SIZE ];//缓存socket发送过来的数据
        fd_set readfds;      //创建写和异常的fd_set
        fd_set exceptfds;
        FD_ZERO( &readfds );  //清空fd_set
        FD_ZERO( &exceptfds );

        while( 1 ){
            memset( buf, 0 , sizeof( buf ));
            //设置要监听读和异常的文件描述符
            FD_SET( connfd, &readfds );
            FD_SET( connfd, &exceptfds );
            //查看connfd是否发生读事件/异常，timeout为NULL表示select阻塞到事件发生
            ret = select( connfd + 1, &readfds, NULL, &exceptfds, NULL );

            if( ret < 0 ){
                printf(" selection faliure\n");
                break;
            }

            if( FD_ISSET( connfd, &readfds )){ //在返回的readfds检查到了connfd， 说明有读事件发生
                ret = recv(connfd, buf, sizeof(buf) - 1, 0);
                if( ret < 0 ){
                    break;
                }
                printf("get %d bytes of normal data: %s\n", ret, buf);

            }
            else if( FD_ISSET( connfd, &exceptfds)){//在返回的readfds检查到了connfd， 说明有读带外事件发生
                ret = recv(connfd, buf, sizeof(buf) - 1, 0);
                if( ret < 0 ){
                    break;
                }
                printf("get %d bytes of oob data: %s\n", ret, buf);
            }
        }

    }
    close( connfd );
    close( listenfd );
    return 0;
}
