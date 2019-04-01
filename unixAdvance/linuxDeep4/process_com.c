/**
 * 不动笔墨不读书系列
 * 探究进程间通信技术，优化数据传输效率
 * 
 * 进程间通信的不同方式及底层实现原理
 * 管道与FIFO
 * 消息队列
 * 信号量
 * 共享内存
 * 
 * 管道 半双工通信 pipe
 * int pipe(int fds[2]);
 * 
 * 打开两个文件描述符分别用于读取fds[0]和写入fds[1]
 * 这两个文件描述符构成了管道的两端
 * 一端写入，一端读出 所有数据采用比特流形式，读取顺序和写入顺序完全一致，且数据流向为单向
 * 父进程向子进程写入 fds[1] 子进程fds[0]可以读取
 * 子进程写入fds[1]，父进程fds[0]可以读取
 * 但是这样做就没有办法确定是读还是写
 * 
 * 所以就有了单向管道，父进程关闭一个文件，子进程关闭另一个文件
 * 
 * 双向管道
 * 
 * 两方向分别创建管道
 * 
 * 每个管道都有一块环状缓冲区。在linux中，它是一个包含16块pipe_buffer结构的数组。在每块结构中
 * page指针指向一块单独的内存页，并由offset和len字段指明当前缓冲里待读取数据的位置和长度
 * 
 * 保证了不大于页框长度(4KB)的数据写入操作的原子性，一次性写入
 * 如果管道的缓冲区已满，内核会将写入进程挂起，直到管道中的数据被读出并有足够的连续空间存放写入数据为止
 * 如果管道的缓冲区是空的，内核会把试图读取管道数据的进程挂起，直到管道中有任何数据被写入时再唤醒
 * 
 * FIFO
 * 
 * 命名管道FIFO，其实现与管道类似，但在创建时需要为其指定文件系统中的一个路径名
 * 该路径对所有进程可见，任何进程都可以用该路径访问管道，从而将自己设置为管道的读取或写入端，实现进程间的通信
 * 
 * 
 * ***/
int mkfifo(const char *pathname,mode_t mode);

/**
 * 管道和FIFO的局限性
 * 比特流，没有消息边界概念，很难实现，由多个读取进程，每个进程读取特定长度的数据
 * 管道读出顺序与写入顺序严格一致，没有优先级概念
 * 内核存储空间，滞留有限
 * 
 * 消息队列
 * 消息队列数据有边界，发送端与接收端能以消息为单位进行交流，而不再是无分隔的字节流，降低逻辑复杂度
 * 
 * 每条消息都包括一个整形的类型标识，接收端可以读取特定类型的消息，而不需要严格按消息写入的顺序读取
 * 这样可使消息优先级的实现非常简单，而且每个进程可以非常方便地只读取自己感兴趣的消息
 * 
 * 创建消息队列
 * ***/
int msgget(key_t key,int flag);
/**
 * key参数为消息队列的标识符
 * 通信双方约定好的整数标识，就像网络应用中约定服务器使用的端口号一样
 * 也可以使用ftok()函数根据约定的文件名生成的整数值
 * 还可以指定IPC_PRIVATE，让内核选定一个未被占用的整数作为标识符，但这种用法需要通过另外的机制通知对方所选定的key
 * 
 * 发送和接收消息 通过ID进行
 * 发送和接受消息
 * ***/
int msgsnd(int msgid,const void *msgp,size_t msgsz,int msgflag);
int msgrcv(int msgid,void *msgp,size_t maxmsgsz,long msgtp,int msgflag);

// man手册查用法
/**
 * 内核结构
 * 消息队列在内核的结构
 * 在利用消息队列实现进程间通信，且通信比较频繁时，应合理控制单个消息体的长度，避免消息分页
 * 可调整消息队列的数量
 * 
 * 信号量
 * 信号量用于协调进程间的运行步调，也叫做进程同步。
 * 经典的生产者消费者问题
 * 用于保护进程间共享的临街资源，类似于在多线程程序中用互斥量保护全局临界区
 * 
 * 信号量在线程互斥量之前已经出现了，信号量常与共享内存配合使用
 * 增加、减少和检查 PV操作
 * 
 * 信号量非零挂起，为零唤醒
 * 信号量的创建和初始化分两步骤进行，当多个平行进程有可能同时运行时，需要特别注意可能出现的竞争条件
 * 
 * 创建与操作
 * ***/
int semget(key_t key,int nsems,int semflag); // 创建一组信号量 nsems指定创建的信号量的数量
int semctl(int semid,int semnum,int cmd,...);// 控制信号量，包括初始化，删除，状态查询
int semop(int semid,struct sembuf *sops,unsigned int nsops);
// 用于堆信号量执行一组操作

/**
 *  /proc/sys/kernel/sem  查看信号量限制🚫
 * 共享内存
 * 
 * 功能最强，应用最广的进程间通信技术
 * 多个进程共享相同物理内存区，一个进程对该内存区的任意修改，可以被其他进程立即看到
 * 线性地址，映射相同的物理内存页
 * 
 * 创建与操作
 * ***/
int shmget(key_t key,size_t size,int shmflg);
void * shmat(int shmid,const void *shmaddr,int shmflg);
int shmdt(const void *shmaddr);
/**
 * shmget函数创建或获取一块指定大小(size)的共享内存，key和shmflg的意义与消息队列函数中的key和flag类似
 * shmat()将指定的共享内存附加到进程的线性地址空间内
 * shmdt()函数用于将共享内存段从当前进程中分离
 * 共享内存通常可与信号量配合使用，实现临界区的一致性保护，除非在其上实现的是某种无锁的数据结构
 * /proc/sys/kernel/shmmni
 * /proc/sys/kernel/shmmax
 * /proc/sys/kernel/shmall
 * ***/
