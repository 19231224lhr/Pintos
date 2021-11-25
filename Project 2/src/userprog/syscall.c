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

static void syscall_handler (struct intr_frame *);
static void * judge_pointer(const void *vaddr);
static int get_user (const uint8_t *uaddr);


/* Reads a byte at user virtual address UADDR.
   UADDR must be below PHYS_BASE.
   Returns the byte value if successful, -1 if a segfault
   occurred. */
static int
get_user (const uint8_t *uaddr)
{
    int result;
    asm ("movl $1f, %0; movzbl %1, %0; 1:"
    : "=&a" (result) : "m" (*uaddr));
    return result;
}

// �ж�ָ��ָ���ڴ�ĺ�����
void *
judge_pointer(const void *vaddr)
{
    void *ptr = pagedir_get_page (thread_current()->pagedir, vaddr);
    /* �ж��Ƿ������û���ַ�ռ䣬�ռ��Ƿ��Ѿ���ӳ�䣨��Ч�ԣ� */
    if (!is_user_vaddr(vaddr) || ptr == NULL)
    {
        // �����߳�״ֵ̬Ϊ-1����ʾ�̴߳���
        thread_current()->st_exit = -1;
        // �߳��˳�
        thread_exit ();
    }
    /* ��ȷ�Χ�ϵ��ж� */
    uint8_t *check_byteptr = (uint8_t *) vaddr;
    // ע�⣬���ӵ�3����Ϊ4��ʱ���Ѿ�������һ���ڴ浥λ
    for (uint8_t i = 0; i < 4; i++)
    {
        // �жϸ�������
        if (get_user(check_byteptr + i) == -1)
        {
            thread_current()->st_exit = -1;
            thread_exit ();
        }
    }
    return ptr;
}



void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");
  thread_exit ();
}
