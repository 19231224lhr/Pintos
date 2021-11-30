#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "process.h"
#include "pagedir.h"
#include <threads/vaddr.h>
#include <filesys/filesys.h>
#include <devices/shutdown.h>
#include <filesys/file.h>
#include <devices/input.h>
#include <threads/malloc.h>
#include <threads/palloc.h>

static void syscall_handler(struct intr_frame *);

static void *judge_pointer(const void *vaddr);

static int get_user(const uint8_t *uaddr);

// 存储系统调用类型的数组syscallArray
static void (*syscallArray[20])(struct intr_frame *);

// 系统调用
void sys_halt(struct intr_frame *f); /* syscall halt. */
void sys_exit(struct intr_frame *f); /* syscall exit. */
void sys_exec(struct intr_frame *f); /* syscall exec. */
void sys_create(struct intr_frame *f); /* syscall create */
void sys_remove(struct intr_frame *f); /* syscall remove */
void sys_open(struct intr_frame *f);/* syscall open */
void sys_wait(struct intr_frame *f); /*syscall wait */
void sys_filesize(struct intr_frame *f);/* syscall filesize */
void sys_read(struct intr_frame *f);  /* syscall read */
void sys_write(struct intr_frame *f); /* syscall write */
void sys_seek(struct intr_frame *f); /* syscall seek */
void sys_tell(struct intr_frame *f); /* syscall tell */
void sys_close(struct intr_frame *f); /* syscall close */

/* Find file by the file's ID */
struct thread_file *
find_file_id (int file_id)
{
    struct list_elem *e;
    struct thread_file * thread_file_temp = NULL;
    struct list *files = &thread_current ()->files;
    for (e = list_begin (files); e != list_end (files); e = list_next (e)){
        thread_file_temp = list_entry (e, struct thread_file, file_elem);
        if (file_id == thread_file_temp->fd)
            return thread_file_temp;
    }
    return false;
}

/* Reads a byte at user virtual address UADDR.
   UADDR must be below PHYS_BASE.
   Returns the byte value if successful, -1 if a segfault
   occurred. */
static int
get_user(const uint8_t *uaddr) {
    int result;
    asm ("movl $1f, %0; movzbl %1, %0; 1:"
    : "=&a" (result) : "m" (*uaddr));
    return result;
}

// 判断指针指向内存的合理性
void *
judge_pointer(const void *vaddr) {
    void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
    /* 判断是否属于用户地址空间，空间是否已经被映射（有效性） */
    if (!is_user_vaddr(vaddr) || ptr == NULL) {
        // 设置线程状态值为-1，表示线程错误
        thread_current()->exit_status = -1;
        // 线程退出
        thread_exit();
    }
    /* 广度范围上的判断 */
    uint8_t *check_byteptr = (uint8_t *) vaddr;
    // 注意，最多加到3，因为4的时候已经跳出了一个内存单位
    for (uint8_t i = 0; i < 4; i++) {
        // 判断辅助函数
        if (get_user(check_byteptr + i) == -1) {
            thread_current()->exit_status = -1;
            thread_exit();
        }
    }
    return ptr;
}

// 系统调用初始化
void
syscall_init(void) {
    // 存储中断类型为系统调用syscall，存储中断对应的操作syscall_handler函数，通过0x30识别为系统调用
    intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
    // 存储系统调用对应的操作，SYS_EXEC为Pintos定义的系统调用标识号
    // 使用的是枚举类中定义的系统调用标识号，因此实际存储的数组为下标从0开始递增的数组
    syscallArray[SYS_EXEC] = &sys_exec;
    syscallArray[SYS_HALT] = &sys_halt;
    syscallArray[SYS_EXIT] = &sys_exit;
    syscallArray[SYS_WAIT] = &sys_wait;
    syscallArray[SYS_WRITE] = &sys_write;
    syscallArray[SYS_CREATE] = &sys_create;
    syscallArray[SYS_OPEN] = &sys_open;
}

/* 检测指针是否正确，检测系统调用号是否正确，执行系统调用 */
static void
syscall_handler(struct intr_frame *f UNUSED) {
    int *ptr = f->esp;
    // 当前如果要执行系统调用，则需要判断指针是否指向正确，这一部分功能我们已经在上面的judge_pointer()函数中实现了，直接调用即可
    // ptr里指向系统调用函数名，ptr + 1里指向系统调用第一个参数
    judge_pointer(ptr + 1);
    // 光指针正确了还不行，因为这样也可以调用不是系统调用的函数，因此，我们需要检测寄存器的值，在src/lib/syscall-nr.h文件中，系统调用总共有20个，因此，只要寄存器的值大于20或小于0，就说明当前调用了系统调用处理函数但是没有调用系统调用，此时程序应该报错并退出
    int type = *(int *) f->esp;
    if (type <= 0 || type >= 20) {
        // 设置线程状态值为-1，表示线程错误
        thread_current()->exit_status = -1;
        // 线程退出
        thread_exit();
    }
    // 上述判断都通过后，可以执行系统调用函数，f为系统调用函数相关参数
    syscallArray[type](f);
    //printf ("system call!\n");
    //thread_exit ();
}

// 系统调用：halt()，关闭系统
// f:void
void
sys_halt(struct intr_frame *f) {
    shutdown_power_off();
}

/*
 * 系统调用：exit()
 * 终止当前用户程序，将状态返回内核。如果进程的父进程等待它（见下文），则将返回此状态。通常，状态为0表示成功，非零值表示错误。
 */
void
sys_exit(struct intr_frame *f) {
    uint32_t *ptr = f->esp;
    // ptr里指向系统调用函数名，ptr + 1里指向系统调用第一个参数
    // 也可以用f->esp + 4来表示ptr指向下一位
    judge_pointer(ptr + 1);
    // 指针移动，指向第一个参数
    *ptr++;
    // 第一个参数保存了int status，将其保存在进程状态中
    thread_current()->exit_status = *ptr;
    // 进程退出
    thread_exit();
}

/*
 * 系统调用：exec()
 * 运行名称在cmd_行中给定的可执行文件，传递任何给定参数，并返回新进程的程序id（pid）。
 * 如果程序由于任何原因无法加载或运行，则必须返回pid-1，否则该pid不应是有效的pid。因此，在知道子进程是否成功加载其可执行文件之前，父进程无法从exec返回。
 * 您必须使用适当的同步来确保这一点。
 */
void
sys_exec (struct intr_frame* f)
{
    uint32_t *ptr = f->esp;
    // ptr中存放的是系统调用函数名称，ptr+1中存放的是系统调用函数参数*cmd_line
    // 也可以用f->esp + 4来表示ptr指向下一位
    judge_pointer (ptr + 1);
    // 重点：参数需要检查，但是参数本身又是一个指针，指向了存放调用函数需要的参数地址，我们也需要检查这个指针指向的地址是否正确
    judge_pointer (*(ptr++));
    // 调用函数process_execute()，函数返回值是进程pid，函数返回值存放在寄存器eax中
    f->eax = process_execute((char*)* ptr);
}

/*
 * 系统调用：wait()
 * 等待子进程pid并检索子进程的退出状态。如果pid仍处于活动状态，则等待它终止。
 * 然后，返回pid传递给exit的状态。如果pid没有调用exit（），但被内核终止（例如，由于异常而终止），那么wait（pid）必须返回-1。
 * 父进程等待在父进程调用wait时已经终止的子进程是完全合法的，但是内核仍然必须允许父进程检索其子进程的退出状态，或者知道子进程已被内核终止。
 */
void
sys_wait (struct intr_frame* f)
{
    uint32_t *ptr = f->esp;
    // ptr存储系统调用函数名称，ptr+1存储系统调用参数指针
    judge_pointer (ptr + 1);
    // 指针移动一位
    *ptr++;
    // 子进程的退出状态值赋值给eax寄存器
    f->eax = process_wait(*ptr);
}

/*
 * 系统调用：write()
 */
void
sys_write (struct intr_frame* f)
{
    uint32_t *user_ptr = f->esp;
    judge_pointer (user_ptr + 7);
    judge_pointer (*(user_ptr + 6));
    *user_ptr++;
    // ptr文件描述符
    int fd = *user_ptr;
    // ptr + 1参数数组
    const char * buffer = (const char *)*(user_ptr+1);
    // ptr + 2参数长度
    off_t size = *(user_ptr+2);
    if (fd == 1) {
        putbuf(buffer,size);
        f->eax = size;
    }
    else
    {
        struct thread_file * thread_file_temp = find_file_id (*user_ptr);
        if (thread_file_temp)
        {
            acquire_lock_f ();
            f->eax = file_write (thread_file_temp->file, buffer, size);
            release_lock_f ();
        }
        else
        {
            f->eax = 0;
        }
    }
}

/*
 * 系统调用：create()
 * 创建一个名为file，initially_size字节的新文件。如果成功，则返回true，否则返回false。
 * 创建新文件不会打开它：打开新文件是一个单独的操作，需要打开系统调用。
 */
/* Do sytem create, we need to acquire lock for file operation in the following methods when do file operation */
void
sys_create(struct intr_frame* f)
{
    uint32_t *ptr = f->esp;
    // 检验指针是否正确
    judge_pointer(ptr + 5);
    judge_pointer(*(ptr + 4));
    *ptr++;
    // 申请锁
    acquire_lock_f ();
    // 注意*ptr此时已经指向了参数中的第二个参数
    f->eax = filesys_create ((const char *)*ptr, *(ptr+1));
    // 释放锁
    release_lock_f ();
}

/*
 * 系统调用：remove()
 * 删除名为file的文件。如果成功，则返回true，否则返回false。
 * 无论文件是打开的还是关闭的，都可以将其删除，删除打开的文件不会将其关闭。
 * 有关详细信息，请参见删除打开的文件。
 */
void
sys_remove(struct intr_frame* f)
{
    uint32_t *ptr = f->esp;
    // ptr：系统调用函数名
    judge_pointer (ptr);
    // ptr + 1：指针，系统调用第二个参数，指向系统调用第二个参数存储的地址
    judge_pointer (ptr + 1);
    // *(ptr + 1)：系统调用第二个参数，要删除的文件名
    judge_pointer (*(ptr + 1));
    // 不要忘记指针向后移一位
    *ptr++;
    // 申请锁
    acquire_lock_f ();
    // 调用filesys_remove()函数，注意将函数返回值放入寄存器eax中
    f->eax = filesys_remove ((const char *)*ptr);
    // 释放锁
    release_lock_f ();
}

/*
 * 系统调用：open()
 * 您必须同步系统调用，以便任意数量的用户进程可以同时进行调用。特别是，一次从多个线程调用filesys目录中提供的文件系统代码是不安全的。
 * 系统调用实现必须将文件系统代码视为关键部分。不要忘记，process_execute（）也会访问文件。目前，我们建议不要修改filesys目录中的代码。
 */
void
sys_open (struct intr_frame* f)
{
    uint32_t *ptr = f->esp;
    // 检查指针正确性
    // ptr + 1指针指向系统调用第二个参数：要打开文件的文件名存放地址
    judge_pointer (ptr + 1);
    // *(ptr + 1)为要打开文件的文件名
    judge_pointer (*(ptr + 1));
    // 不要忘记指针向下移动一位
    *ptr++;
    // 申请锁
    acquire_lock_f ();
    // 调用filesys_open()函数
    struct file * file_a = filesys_open((const char *)*ptr);
    // 释放锁
    release_lock_f ();
    // 准备为当前进程添加刚刚打开的文件资源，先定义当前进程结构体
    struct thread * t = thread_current();
    if (file_a)
    {
        // 先定义进程文件结构体，并为其申请存储空间
        struct thread_file *file_b = malloc(sizeof(struct thread_file));
        // 赋值文件fd，利用了fd从小到大的特点
        file_b->fd = t->all_fd++;
        // 添加文件资源
        file_b->file = file_a;
        // 将复制好的进程文件结构体放入当前进程的files列表中
        list_push_back (&t->files, &file_b->file_elem);//维护files列表
        // 根据题意，函数返回打开文件的文件描述符，因此将打开文件的文件描述符fd放到寄存器eax中
        f->eax = file_b->fd;
    }
    else
    {
        // 文件无法打开，返回-1
        f->eax = -1;
    }
}

/*
 * 系统调用：filesize()
 * 返回作为fd打开的文件的大小（以字节为单位）。
 */
void
sys_filesize (struct intr_frame* f){
    uint32_t *ptr = f->esp;
    // ptr + 1:fd
    judge_pointer (ptr + 1);
    *ptr++;
    // 文件fd对应的文件结构体
    struct thread_file * file_a = find_file_id (*ptr);
    if (file_a)
    {
        // 申请锁
        acquire_lock_f ();
        // 返回对应的文件长度
        f->eax = file_length (file_a->file);
        // 释放锁
        release_lock_f ();
    }
    else
    {
        // 无法打开对应文件，返回-1
        f->eax = -1;
    }
}