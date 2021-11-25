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

// �洢ϵͳ�������͵�����syscallArray
static void (*syscallArray[max_syscall])(struct intr_frame *);

// ϵͳ����
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

// �ж�ָ��ָ���ڴ�ĺ�����
void *
judge_pointer(const void *vaddr) {
    void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
    /* �ж��Ƿ������û���ַ�ռ䣬�ռ��Ƿ��Ѿ���ӳ�䣨��Ч�ԣ� */
    if (!is_user_vaddr(vaddr) || ptr == NULL) {
        // �����߳�״ֵ̬Ϊ-1����ʾ�̴߳���
        thread_current()->st_exit = -1;
        // �߳��˳�
        thread_exit();
    }
    /* ��ȷ�Χ�ϵ��ж� */
    uint8_t *check_byteptr = (uint8_t *) vaddr;
    // ע�⣬���ӵ�3����Ϊ4��ʱ���Ѿ�������һ���ڴ浥λ
    for (uint8_t i = 0; i < 4; i++) {
        // �жϸ�������
        if (get_user(check_byteptr + i) == -1) {
            thread_current()->st_exit = -1;
            thread_exit();
        }
    }
    return ptr;
}

// ϵͳ���ó�ʼ��
void
syscall_init(void) {
    // �洢�ж�����Ϊϵͳ����syscall���洢�ж϶�Ӧ�Ĳ���syscall_handler������ͨ��0x30ʶ��Ϊϵͳ����
    intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
    // �洢ϵͳ���ö�Ӧ�Ĳ�����SYS_EXECΪPintos�����ϵͳ���ñ�ʶ��
    // ʹ�õ���ö�����ж����ϵͳ���ñ�ʶ�ţ����ʵ�ʴ洢������Ϊ�±��0��ʼ����������
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

/* ���ָ���Ƿ���ȷ�����ϵͳ���ú��Ƿ���ȷ��ִ��ϵͳ���� */
static void
syscall_handler (struct intr_frame *f UNUSED)
{
    int * ptr = f->esp;
    // ��ǰ���Ҫִ��ϵͳ���ã�����Ҫ�ж�ָ���Ƿ�ָ����ȷ����һ���ֹ��������Ѿ��������judge_pointer()������ʵ���ˣ�ֱ�ӵ��ü���
    judge_pointer (ptr + 1);
    // ��ָ����ȷ�˻����У���Ϊ����Ҳ���Ե��ò���ϵͳ���õĺ�������ˣ�������Ҫ���Ĵ�����ֵ����src/lib/syscall-nr.h�ļ��У�ϵͳ�����ܹ���20������ˣ�ֻҪ�Ĵ�����ֵ����20��С��0����˵����ǰ������ϵͳ���ô���������û�е���ϵͳ���ã���ʱ����Ӧ�ñ����˳�
    int type = * (int *)f->esp;
    if(type <= 0 || type >= 20){
        // �����߳�״ֵ̬Ϊ-1����ʾ�̴߳���
        thread_current()->st_exit = -1;
        // �߳��˳�
        thread_exit ();
    }
    // �����ж϶�ͨ���󣬿���ִ��ϵͳ���ú�����fΪϵͳ���ú�����ز���
    syscallArray[type](f);
}