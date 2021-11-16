

# Project 2

## Task 1：Process Termination Messages

在process.c中修改setup_stack函数中*esp = PHYS_BASE为*esp = PHYS_BASE-12 。这一步是为了人为给系统调用参数留下空间，防止访问越界。

在thread结构体中加入变量ret_state用于保存返回值

```c
struct thread{
    int ret_state;//保存进程退出状态
}
```



在process.c中的process_exit（）函数中打印终止信息

```c
void
thread_exit (void) {
    printf ("%s: exit(%d)\n",cur->name, cur->ret); /* 打印name以及return值 */ 
}
```



## Task 2：Argument Passing

在thread结构体中加入以下变量。

```c
uint32_t *pagedir;          /* Page directory. */
struct semaphore sema;     //信号量
struct thread* parent;     //父进程
bool success;              //记录线程是否成功执行
```

在process.c中的`process_execute()`中，先将传入的字符串file_name复制一份到fn_copy，然后用`strtok_r()`函数将字符串file_name分割，获得线程名。用线程名创建新的线程，并让该线程执行`start_process()`函数，并且向该线程传递参数fn_copy。

```c
tid_t
process_execute (const char *file_name) 
{
  char *fn_copy;
  tid_t tid;

  /* Make a copy of FILE_NAME.
     Otherwise there's a race between the caller and load(). */
  fn_copy = palloc_get_page (0); //获取单个空闲页并返回其内核虚拟地址
  if (fn_copy == NULL)
    return TID_ERROR;
  strlcpy (fn_copy, file_name, PGSIZE);
  
  char *save_ptr;
  file_name = strtok_r(file_name," ",&save_ptr);  //分离输入的字符串，得到要新建进程的进程名
  /* Create a new thread to execute FILE_NAME. */
  /*以输入的进程名，新建进程将运行start_process函数，
  fn_copy是被传递参数*/
  tid = thread_create (file_name, PRI_DEFAULT, start_process, fn_copy); 
  if (tid == TID_ERROR)
    palloc_free_page (fn_copy); 
  sema_down(&thread_current()->sema);
  if(!thread_current()->success)
      return TID_ERROR;
  return tid;
}
```

在process.c中`start_process()`中，先将传入的参数file_name_赋值给file_name字符串，然后复制file_name到fn_copy，然后将字符串file_name分割获得进程名file_name。调用`load()`函数,`load()`函数为当前用户程序分配内存，初始化页目录。之后调用`setup_stack`创建用户栈，并把参数传入`setup_stack`。如果调用`load()`成功则将argv[]压入栈，再调用`push_argument()`函数。

```c
/* A thread function that loads a user process and starts it
   running. */
static void
start_process (void *file_name_)
{
  char *file_name = file_name_;    
  struct intr_frame if_;
  bool success;

  char *fn_copy=malloc(strlen(file_name)+1);
  strlcpy (fn_copy, file_name, strlen(file_name)+1);        //保存初始字符串

  char *token, *save_ptr;
  file_name = strtok_r(file_name," ",&save_ptr);    //filename被分割

  /* Initialize interrupt frame and load executable.*/
  //初始化中断帧并加载可执行文件
  memset (&if_, 0, sizeof if_);
  if_.gs = if_.fs = if_.es = if_.ds = if_.ss = SEL_UDSEG;
  if_.cs = SEL_UCSEG;
  if_.eflags = FLAG_IF | FLAG_MBS;
  success = load (file_name, &if_.eip, &if_.esp);

  /* If load failed, quit. */
  palloc_free_page (file_name);
  if (!success) 
  {
    sema_up(&thread_current()->parent->sema);  //父进程的信号量增加
    thread_current()->parent->success = false;    //父进程的执行状态为失败
    thread_exit ();
  }
  else{
    int argc = 0;
    int argv[10];

    for(token = strtok_r(fn_copy," ",&save_ptr);token!=NULL;token = strtok_r(NULL," ",&save_ptr)){
      if_.esp -=(strlen(token)+1);  //栈指针向下移动 +1是因为有\0
      memcpy(if_.esp,token,strlen(token)+1);   //将参数和'\0'复制进栈
      //先是第一个参数+'\0'入栈，然后是第二个
      argv[argc++] = (int)if_.esp;   //argv中存放参数对应的参数的地址
    }
    push_argument(&if_.esp,argc,argv);
  
    sema_up(&thread_current()->parent->sema);  //父进程的信号量增加
    thread_current()->parent->success = true;  //父进程的执行状态为成功
  }
  palloc_free_page(file_name);
  free(fn_copy);

  asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (&if_) : "memory");
  NOT_REACHED ();
}
```

在process.c中`push_argument()`中将argv[]参数的数组压入栈。

```c
void
push_argument(void **esp, int argc, int argv[]){
  *esp = (int)*esp & 0xfffffffc;
  *esp -= 4;
  *(int *) *esp = 0;
  for (int i= argc - 1; i>=0;i--){
    *esp -= 4;
    *(int *) *esp = argv[i];
  }
  *esp -= 4;
  *(int *) *esp = (int) *esp +4;
  *esp -= 4;
  *(int *) *esp = argc;
  *esp -= 4;
  *(int *) *esp = 0;
}
```

