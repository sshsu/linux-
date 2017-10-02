#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<stdio.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<stdlib.h>
#include<fcntl.h>

#define BUFFER_SIZE 4096
//请求行,分析头字段
enum CHECK_STATE {CHECK_STATE_REQUEATLINE = 0, CHECK_STATE_HEADER };
//行完整，行出错，行未完全
enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN};
//请求不完整，请求完整，请求语法错误，无权请求资源，服务器内部错误，客户端关闭连接
enum HTTP_CODE { NO_REQUEST, GET_REQUEST, BAD_REQUEST, FORBIDDEN_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION };

static const char *szret[] = { "I get a correct result\n", "sometthing wrong\n" };

LINE_STATUS  parse_line( char * buffer, int& checked_index, int& read_index){
    char temp;

    for( ; checked_index < read_index ; ++checked_index){

        temp = buffer[checked_index];

        //每一行的结束是以\r\n结束的
        if(  temp == '\r' ) {//当前字符是回车
            if (checked_index + 1 == read_index) {//还未读完
                return LINE_OPEN;
            } else if (buffer[checked_index + 1] == '\n') {//读完一行了，将\r\n置为\0
                buffer[ checked_index++ ] = '\0';
                buffer[ checked_index++ ] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
        //如果遇到了\n，可能上一次检查到'\r'，所以要后退检查一步
        else if( temp == '\n'){
            if( ( checked_index > 1 ) && buffer[ checked_index -1 ] == '\r' ){
                buffer[ checked_index -1 ] = '\0';
                buffer[ checked_index++ ] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

HTTP_CODE parse_requestline(char * temp, CHECK_STATE& checkstate){
    //检查是否有空白字符或者\t字符
    char *url = strpbrk( temp, " \t");
    if( !url ){
        return BAD_REQUEST;
    }

    //将空格置为0
    *(url++) = '\0';

    char *method = temp;
    //分析方法是否为get
    if( strcasecmp( method, "GET") == 0 ){
        printf("The request method is GET\n");
    }
    else{
        return BAD_REQUEST;
    }


    url += strspn( url, " \t");            //定位到第二个字段
    char *version = strpbrk( url, " \t");//定位到第二个字段末

    if( !version ){//没有找到版本字段
        return BAD_REQUEST;
    }

    *( version++ ) = '\0';              //将第二个字段末的空格置0
    version += strspn( version, " \t");//定位到第三个字段头

    if( strcasecmp( version, "HTTP/1.1" ) != 0){
        return BAD_REQUEST;
    }

    if( strncasecmp( url, "http://", 7) == 0 ){
        url += 7;
        url = strchr( url, '/');
    }

    if( !url || url[ 0 ] != '/' ){
        return BAD_REQUEST;
    }

    printf( "THE request URL is: %s\n", url);
    checkstate = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

HTTP_CODE parse_headers( char *temp ){
    if( temp[ 0 ]  == '\0' ){
        return GET_REQUEST;
    }
    else if( strncasecmp( temp, "Host:", 5) == 0 ){
        temp += 5;
        temp += strspn( temp, " \t");
        printf( "the request host is :%s \n", temp);
    }
    else{
        printf( "I can handle this header \n");
    }
    return NO_REQUEST;
}

HTTP_CODE  parse_content( char *buffer, int& checked_index, CHECK_STATE& checkstate,
                          int& read_index, int& start_line ){
    LINE_STATUS linestatus = LINE_OK;               //记录当前行的状态
    HTTP_CODE retcode = NO_REQUEST;                 //记录http请求的处理结果

    while( ( linestatus = parse_line( buffer, checked_index, read_index)) == LINE_OK ){
        char *temp  = buffer + start_line;          //当前行的起始位置
        start_line = checked_index;                 //记录下一行的起始位置
        switch ( checkstate ){
            case CHECK_STATE_REQUEATLINE:{          //分析请求行
                retcode = parse_requestline( temp, checkstate );
                if( retcode == BAD_REQUEST ){
                    return BAD_REQUEST;
                }
                break;
            }
            case CHECK_STATE_HEADER:{               //分析头部字段

                retcode = parse_headers( temp );
                if( retcode == BAD_REQUEST ){
                    return BAD_REQUEST;
                }
                else if ( retcode == GET_REQUEST ){
                    return GET_REQUEST;
                }
                break;
            }
            default:{
                return INTERNAL_ERROR;
            }

        }
    }

    //行为完整，继续返回阻塞
    if( linestatus == LINE_OPEN ){
        return NO_REQUEST;
    }
    else{
        return BAD_REQUEST;
    }
}


int main( int argc, char *argv[] ) {

    if( argc < 3 ){
        printf("usage: %s ip_address, port_number", basename( argv[0] ));
        return 1;
    }
    // 获取参数
    const char * ip = argv[1];
    int port = atoi( argv[2] );

    //创建socket地址
    struct sockaddr_in server_address;
    bzero( &server_address, sizeof( server_address ) );
    server_address.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &server_address.sin_addr );
    server_address.sin_port = htons( port );

    //创建socket
    int sockfd = socket( AF_INET, SOCK_STREAM, 0 );
    assert( sockfd >= 0 );

    //绑定
    int ret = bind( sockfd, ( struct sockaddr* )&server_address, sizeof( server_address ));
    assert( ret !=-1 );

    //监听
    ret = listen( sockfd, 5 );
    assert( ret != -1 );

    sockaddr_in client_address;
    socklen_t  client_address_len = sizeof( client_address );
    int connfd = accept( sockfd, ( struct sockaddr* )&client_address, &client_address_len);
    if( connfd < 0 ){
        printf( "errno is :%d \n ", errno );
    }
    else{
        char* buffer = new char [ BUFFER_SIZE ];
        memset( buffer, '\0', BUFFER_SIZE );
        int data_read = 0;
        int read_index = 0;
        int checked_index = 0;
        int start_line = 0;

        CHECK_STATE  checkstate = CHECK_STATE_REQUEATLINE;
        while(1) {
            data_read = recv( connfd, buffer+read_index, BUFFER_SIZE - read_index, 0);
            if( data_read == -1){
                printf( "reading failed\n ");
                break;
            }
            else if( data_read == 0 ){
                printf( "remote client has closed the connection \n");
                break;
            }
            read_index += data_read;

            HTTP_CODE result = parse_content( buffer, checked_index, checkstate, read_index, start_line);

            if( result == NO_REQUEST ){
                continue;
            }
            else if ( result  == GET_REQUEST) {
                send( connfd, szret[0], strlen( szret[0] ), 0 );
                break;
            }
            else {
                send(connfd, szret[1], strlen(szret[1]), 0);
                break;
            }
        }
        close( connfd );
    }
    close( sockfd );
    return 0;
}