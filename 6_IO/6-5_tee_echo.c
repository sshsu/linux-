/*
	此程序是一个同时输出数据到终端和文件的程序,通过splice获取标准输入的内容到管道之后
	使用tee将内容从管道拷贝一份到另一个管道pipefile，然后使用splice将内容导入到文件中
	使用splice将内容导入到标准输出中

	函数原型
	#include<fcntl.h>
	ssize_t tee(int fd_in, int fd_out, size_t len, unsigned int flags);

	tee函数在两个管道文件描述符之间父子数据，也是零拷贝操作。并且不消耗数据。

	fd_in:	提供数据文件
	fd_out: 待写入数据的文件
	len: 	写入的数据的长度
	flags: 	控制数据如何移动的,多个值一起使用的时候，用按位或进行
		|== SPLICE_F_MOVE: 给内核一个提示，合适的话，整页移动数据，自2.6.21后，没有效果了
		|== SPLICE_F_NONBLOCK: 非阻塞的splice操作，但实际效果还会收文件描述符本身的阻塞状态影响
		|== SPLICE_F_MORE: 给内核一个提示，后续的splice调用将读取更多数据
		|== SPLICE_F_GIFT: 无效果

	fd_in和fd_out必须都是管道文件描述符。tee函数成功是返回在两个文件描述符之间复制的数据数量(字节数)
	返回0表示没有复制任何数据。tee失败时返回01并设置errno

*/

#include<assert.h>
#include<stdio.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<fcntl.h>

int main( int argc, char *argv[] ){

	if( argc != 2 ){
		printf( "usage: %s filename \n", argv[0] );
		return 1;
	}

	//打开文件
	int filefd = open( argv[1], O_CREAT | O_WRONLY | O_TRUNC, 0666 );
	assert( filefd > 0 );

	//创建两组管道文件描述符
	//读取标准输入的管道文件
	int pipefd_stdout[2]; 
	int ret = pipe( pipefd_stdout );
	assert( ret != -1 );

	//数据的缓存管道文件
	int pipefd_file[2]; 
	ret = pipe( pipefd_file );
	assert( ret != -1 );

	//将标准输入的内容导入到pipefd_stdout中
	ret = splice( STDIN_FILENO, NULL, pipefd_stdout[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE );
	assert( ret != -1 );

	
	//使用tee将内容拷贝一份到pipefd_file中
	ret = tee( pipefd_stdout[0], pipefd_file[1], 32768, SPLICE_F_MORE | SPLICE_F_MOVE );
	assert( ret != -1 );
	//将数据导入到文件中
	ret = splice( pipefd_file[0], NULL, filefd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE );
	assert( ret != -1 );
	

	//最后将数据导入到标准输出中
	ret = splice( pipefd_stdout[0], NULL, STDOUT_FILENO, NULL, 3768, SPLICE_F_MORE | SPLICE_F_MOVE );
	assert ( ret != -1 );

	close( filefd );
	close( pipefd_file[0] );
	close( pipefd_file[1] );
	close( pipefd_stdout[0] );
	close( pipefd_stdout[1]);
return 0;
}