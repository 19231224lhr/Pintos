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
static void (*syscallArray[max_syscall])(struct intr_frame *);

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
        thread_current()->st_exit = -1;
        // 线程退出
        thread_exit();
    }
    /* 广度范围上的判断 */
    uint8_t *check_byteptr = (uint8_t *) vaddr;
    // 注意，最多加到3，因为4的时候已经跳出了一个内存单位
    for (uint8_t i = 0; i < 4; i++) {
        // 判断辅助函数
        if (get_user(check_byteptr + i) == -1) {
            thread_current()->st_exit = -1;
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
    syscallArray[SYS_CREATE] = &sys_create;
    syscallArray[SYS_REMOVE] = &sys_remove;
    syscallArray[SYS_OPEN] = &sys_open;
    syscallArray[SYS_WRITE] = &sys_write;
    syscallArray[SYS_SEEK] = &sys_seek;
    syscallArray[SYS_TELL] = &sys_tell;
    syscallArray[SYS_CLOSE] = &sys_close;
    syscallArray[SYS_READ] = &sys_read;
    syscallArray[SYS_FILESIZE] = &sys_filesize;
}

/* 检测指针是否正确，检测系统调用号是否正确，执行系统调用 */
static void
syscall_handler (struct intr_frame *f UNUSED)
{
    int * ptr = f->esp;
    // 当前如果要执行系统调用，则需要判断指针是否指向正确，这一部分功能我们已经在上面的judge_pointer()函数中实现了，直接调用即可
    judge_pointer (ptr + 1);
    // 光指针正确了还不行，因为这样也可以调用不是系统调用的函数，因此，我们需要检测寄存器的值，在src/lib/syscall-nr.h文件中，系统调用总共有20个，因此，只要寄存器的值大于20或小于0，就说明当前调用了系统调用处理函数但是没有调用系统调用，此时程序应该报错并退出
    int type = * (int *)f->esp;
    if(type <= 0 || type >= 20){
        // 设置线程状态值为-1，表示线程错误
        thread_current()->st_exit = -1;
        // 线程退出
        thread_exit ();
    }
    // 上述判断都通过后，可以执行系统调用函数，f为系统调用函数相关参数
    syscallArray[type](f);
}