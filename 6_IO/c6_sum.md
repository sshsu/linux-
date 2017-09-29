### <center>chapter 6 高级IO函数</center>

### pipe
创建管道文件, 参数是一个数组,fd[0]和fd[1]分别存放着写端和读端，成功返回0，失败返回-1并设置errno,是读写阻塞的,默认读写容量
为65536字节

```python
	#include<unistd.h>
	int pipe( int fd[2]);

	-----------------------------
	int fd[2];
	int ret = pipe( fd );
	assert( ret != -1 );
```

### socketpair
双向管道，成功的时候返回0，失败的时候返回-1并上设置errno
domain, type, protocol 与socket()函数一样,但domain只能使用AF_UNIX
```
	#include<sys/types.h>
	#include<sys/socket.h>
	int socketpair( int domain, int type, int protocol, int fd[2]);

	-----------------------------
	int fd[2]
	int ret = socket( AF_UNIX, SOCK_STREAM, 0, fd );
	assert( ret != -1 );
```

### dup和dup2函数
dup 函数使用当前进程的最小可用的文件描述符存放被拷贝的文件描述符的内容
成功返回文件描述符，失败返回-1并设置errno。
```
	#include<unistd.h>
	int dup( int file_descriptor);

	-----------------------------
	int filefd = open (filename, O_CREAT | O_WRONLY | O_TRUNC, 0666 );
	int filefd_copy = dup( filefd );
	assert( filefd_copy );

```
</br>
dup2函数与dup函数一样，不同的是返回的不是当前进程最小可用的文件描述符，而是不小于file_descriptor_two的描述符
```
	#include<unnistd.h>
	int dup2( int file_descriptor_one, int file_descriptor_two );

-----------------------------

```

### redv和writev
redv将数据从文件描述符读到分散的内存块中
writev从多块分散的内存块中读取数据并写到一个文件描述符中

```
	#include<sys/uio.h>
	ssize_t readv( int fd, const struct iovec* vector, int count );
	ssize_t writev( int fd, const struct iovec* vector, int count );

	fd:要进行操作（read/write）的文件描述符
		
	vector:指向的多块独立内存地址

	count: 一共有多少块内存地址

	因为操作的是内存地址向量所以名称为 writev(write vector)

	-----------------------------

	struct iovec iv[2];
	iv[ 0 ].iov_base = header_buf;
	iv[ 0 ].iov_len = strlen ( header_buf );
	iv[ 1 ].iov_base = file_buf;
	iv[ 1 ].iov_len = file_stat.st_size;
	ret = writev( connfd, iv, 2);

```

### sendfile
在内核中直接在两个文件描述符之间进行数据传递，避免了内核缓冲区和用户缓冲区之间的数据拷贝，成为零拷贝
in_fd必须是一个指向真实文件的描述符，out_fd必须是一个socket

```
	#include<sys/sendfile.h>
	ssize_t sendfile( int out_fd, int in_fd, off_t* offset, size_t count)

	out_fd: 待写入数据的文件描述符，必须是一个socket
	in_fd: 指向一个真实的文件的描述符，不能是socket或管道
		 |----offset:从in_fd那个位置开始读
		 |----count:从in_fd传了多少到out_fd中	  

-----------------------------

sendfile( connfd, filefd, 0, stat_buf.st_size );

```

### splice
文件描述符之间的数据传输，会消耗数据，fd_in或fd_out必须有一个是管道文件描述符

```
	#include<fcntl.h>
	ssize_t splice( int fd_in, loff_t* off_in, int fd_out, loff_t* off_out, size_t len,
					 unsigned int flags);
	fd_in:输入数据文件描述符
		|---off_in: 如果fd_in是一个管道文件，那么off_in设为NULL, 否则表示从输入数据流何处开始读数据
	fd_out:输出文件描述符
		|---off_out: 如果fd_out是一个管道文件，那么off_out设为NULL,否则是数据写入的偏移
	len:指定移动数据的长度
	flags: 控制数据如何移动的,多个值一起使用的时候，用按位或进行
		|== SPLICE_F_MOVE: 给内核一个提示，合适的话，整页移动数据，自2.6.21后，没有效果了
		|== SPLICE_F_NONBLOCK: 非阻塞的splice操作，但实际效果还会收文件描述符本身的阻塞状态影响
		|== SPLICE_F_MORE: 给内核一个提示，后续的splice调用将读取更多数据
		|== SPLICE_F_GIFT: 无效果

	使用splice函数时，fd_in和fd_out必须至少有一个是管道文件描述符。splice函数调用成功时返回移动字节的数量。
	失败的时候返回-1,并设置errno
	errno
		|== EBADF: 参数所指描述符有错
		|== EINVAL:系统不支持splice,或文件以追加方式打开，或两个文件描述符都不是管道文件描述符，或offset被用于不支持随机访问的设备。
		|== ENOMEM:内存不够
		|== ESPIPE:参数fd_in(fd_out)是管道文件描述符，而off_in(或off_out)不为NULL.

-----------------------------
		int pipefd[2];
		assert( ret != -1 );
		//创建管道
		ret = pipe( pipefd );
		//将socket中的数据定向到管道写端fd[1]中去
		ret = splice( connfd, NULL, pipefd[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE );
		assert( ret != -1 );
```

### tee
文件描述符之间的数据传输，不会消耗数据，相当于拷贝。
fd_in和fd_out必须都是管道文件描述符。tee函数成功是返回在两个文件描述符之间复制的数据数量(字节数)
返回0表示没有复制任何数据。tee失败时返回-1并设置errno

```
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



```