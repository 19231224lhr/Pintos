## Project 2实验手册

> 

现在是时候开始着手处理系统中允许**运行用户程序**的部分了。基本代码已经支持加载和运行用户程序，但不支持I/O或交互。在这个项目中，您将允许程序通过系统调用与操作系统交互。

您将在**userprog目录**下完成此任务，但您也将与Pintos的几乎所有其他部分进行交互。我们将在下面描述相关部分。

您可以在项目1提交的基础上构建项目2，也可以重新开始。**此任务不需要项目1中的代码**。“闹钟”功能在项目3和项目4中可能有用，但并非严格要求。

在上一个项目中，我们将测试代码直接编译到内核中，因此我们必须在内核中要求特定的函数接口。从现在起，我们将通过运行用户程序来测试您的操作系统。这给了你更大的自由。您必须确保用户程序界面符合此处描述的规范，但考虑到该约束，您可以随意重新构造或重写内核代码。

您将需要与此项目的文件系统代码进行接口，因为用户程序是从文件系统加载的，并且您必须实现的许多系统调用都与文件系统进行处理。但是，本项目的重点不是文件系统，因此我们在**filesys目录**中提供了一个简单但完整的文件系统。您需要查看**`filesys.h`和`file.h`接口**，以了解如何使用文件系统，尤其是它的许多限制。
不需要修改此项目的文件系统代码，因此我们建议您不要修改。在文件系统上工作可能会分散您对本项目重点的注意力。

### 建议实现顺序

- 参数传递（参见第3.3.3节参数传递）
- 用户内存访问（参见第3.1.5节访问用户内存）。所有系统调用都需要读取用户内存。很少有系统调用需要写入用户内存。
- 系统调用基础设施（参见第3.3.4节系统调用）。实现足够的代码，以便从用户堆栈读取系统调用号，并基于它分派给处理程序。
- 退出系统调用。以正常方式完成的每个用户程序都调用exit。即使是从main（）返回的程序也会间接地调用exit（参见lib/user/entry.c中的_start（））。
- 写入系统调用，用于写入系统控制台fd 1。我们所有的测试程序都会写入控制台（printf（）的用户进程版本就是这样实现的），因此在写入可用之前，它们都会出现故障。
- 现在，将进程_wait（）更改为无限循环（一个永远等待的循环）。提供的实现会立即返回，因此Pintos将在任何进程实际运行之前关闭。您最终需要提供一个正确的实现。

### 任务介绍

- **3.3.2过程终止消息**
  每当用户进程因调用exit或任何其他原因而终止时，请打印该进程的名称和退出代码，其格式与打印F相同（“%s:exit（%d）\n”，…）；。打印的名称应该是传递给进程_execute（）的全名，忽略命令行参数。当不是用户进程的内核线程终止或调用halt系统调用时，不要打印这些消息。当进程加载失败时，该消息是可选的。
  除此之外，不要打印Pintos提供的尚未打印的任何其他邮件。在调试期间，您可能会发现额外的消息很有用，但它们会混淆分级脚本，从而降低您的分数。

- **3.3.3参数传递**

  当前，process_execute（）不支持向新进程传递参数。通过扩展process_execute（）来实现此功能，这样它就不用简单地将程序文件名作为参数，而是在空格处将其划分为单词。第一个字是程序名，第二个字是第一个参数，依此类推。也就是说，process_execute（“grep foo bar”）应该通过两个参数foo和bar运行grep。

- **系统调用**

- 在`userprog/syscall.c`中实现系统调用处理程序。我们提供的框架实现通过终止进程来“处理”系统调用。它需要检索系统调用号，然后检索任何系统调用参数，并执行适当的操作。
  - **void halt（void）**
    通过调用shutdown_power_off（）（在`devices/shutdown.h`中声明）终止Pintos。这应该很少使用，因为您会丢失一些关于可能的死锁情况等的信息。
    
  - **void exit (int status)**
    终止当前用户程序，将状态返回内核。如果进程的父进程等待它（见下文），则将返回此状态。通常，状态为0表示成功，非零值表示错误。
    
  - **pid_t exec (const char *cmd_line)**
    运行名称在cmd_line中给定的可执行文件，传递任何给定参数，并返回新进程的程序id（pid）。如果程序由于任何原因无法加载或运行，则必须返回pid-1，否则该pid不应是有效的pid。因此，在知道子进程是否成功加载其可执行文件之前，父进程无法从exec返回。您必须使用适当的同步来确保这一点。
  
  - **int wait (pid_t pid)**
  
    等待子进程pid并检索子进程的退出状态。
  
    如果pid仍处于活动状态，则等待它终止。然后，返回pid传递给exit的状态。如果pid没有调用exit（），但被内核终止（例如，由于异常而终止），那么wait（pid）必须返回-1。父进程等待在父进程调用wait时已经终止的子进程是完全合法的，但是内核仍然必须允许父进程检索其子进程的退出状态，或者知道子进程已被内核终止。
  
  - **bool create (const char *file, unsigned initial_size)**
  
    创建一个名为file initially_size字节的新文件。如果成功，则返回true，否则返回false。创建新文件不会打开它：打开新文件是一个单独的操作，需要打开系统调用。
  
  - **bool remove (const char *file)**
  
    删除名为file的文件。如果成功，则返回true，否则返回false。无论文件是打开的还是关闭的，都可以将其删除，删除打开的文件不会将其关闭。有关详细信息，请参见删除打开的文件。
  
  - **int open (const char *file)**
  
    打开名为file的文件。返回称为“文件描述符”（fd）的非负整数句柄，如果无法打开文件，则返回-1。
  
    编号为0和1的文件描述符为控制台保留：fd 0（标准输入文件号）为标准输入，fd 1（标准输出文件号）为标准输出。开放系统调用永远不会返回这些文件描述符中的任何一个，它们仅作为系统调用参数有效，如下所述。
  
    每个进程都有一组独立的文件描述符。子进程不会继承文件描述符。
  
    当单个文件被多次打开时，无论是由单个进程还是不同的进程打开，每次打开都会返回一个新的文件描述符。
  
  - **int filesize (int fd)**
  
    返回作为fd打开的文件的大小（以字节为单位）。
  
  - **int read (int fd, void *buffer, unsigned size)**
  
    从作为fd打开的文件中读取大小字节到缓冲区中。返回实际读取的字节数（文件末尾为0），如果无法读取文件（由于文件结尾以外的条件），则返回-1。Fd 0使用input_getc（）从键盘读取数据。
  
  - **int write (int fd, const void *buffer, unsigned size)**
  
    将大小字节从缓冲区写入打开的文件fd。返回实际写入的字节数，如果某些字节无法写入，则可能小于大小。
  
  - **void seek (int fd, unsigned position)**
  
    将打开文件fd中要读取或写入的下一个字节更改为位置，以文件开头的字节表示。（因此，位置0是文件的起点。）
    超过文件当前结尾的搜索不是错误。稍后的读取获得0字节，表示文件结束。稍后的写入扩展文件，用零填充任何未写入的间隙。（然而，在Pintos中，在项目4完成之前，文件的长度是固定的，因此写入文件末尾将返回一个错误。）这些语义在文件系统中实现，在系统调用实现中不需要任何特殊的工作。
  
  - **unsigned tell (int fd)**
  
    返回打开文件fd中要读取或写入的下一个字节的位置，以从文件开头开始的字节数表示。
  
  - **void close (int fd)**
  
    关闭文件描述符fd。退出或终止进程会隐式关闭其所有打开的文件描述符，就像为每个描述符调用此函数一样。

该文件定义了其他系统调用。暂时忽略它们。您将在项目3中实现其中一些，在项目4中实现其余部分，因此请确保在设计系统时考虑可扩展性。

> 要实现系统调用，您需要提供在用户虚拟地址空间中读取和写入数据的方法。在获得系统调用号之前，您需要此功能，因为系统调用号位于用户虚拟地址空间中的用户堆栈上。这可能有点棘手：如果用户提供了无效的指针、指向内核内存的指针或其中一个区域的部分块，该怎么办？您应该通过终止用户进程来处理这些情况。我们建议在实现任何其他系统调用功能之前编写和测试此代码。有关更多信息，请参阅第3.1.5节访问用户内存。

您必须同步系统调用，以便任意数量的用户进程可以同时进行调用。特别是，一次从多个线程调用filesys目录中提供的文件系统代码是不安全的。系统调用实现必须将文件系统代码视为关键部分。不要忘记，process_execute（）也会访问文件。目前，我们建议不要修改filesys目录中的代码。

我们在`lib/user/syscall.c`中为每个系统调用提供了一个用户级函数。这些为用户进程提供了一种从C程序调用每个系统调用的方法。每种方法都使用少量内联汇编代码来调用系统调用，并（如果合适）返回系统调用的返回值。

- **3.3.5拒绝写入可执行文件**
  添加代码以拒绝写入用作可执行文件的文件。许多操作系统之所以这样做，是因为如果进程试图运行磁盘上正在更改的代码，会产生不可预测的结果。一旦在项目3中实现了虚拟内存，这一点就显得尤为重要，但即使现在也不会有什么坏处。
  您可以使用file_deny_write（）防止写入打开的文件。对该文件调用file_allow_write（）将重新启用它们（除非该文件被另一个打开程序拒绝写入）。关闭文件还将重新启用写入。因此，要拒绝对进程可执行文件的写入，必须在进程仍在运行时保持其打开状态。

### 目标开始

#### 系统调用是什么？

在 Pintos 中，用户程序调用整数 $0x30进行系统调用，此时用户就会把没有权限干的活交给系统调用去干，系统调用的栈指针就是`esp`，返回值是`eax`。**我们需要干的事说白了就是根据`esp`指向栈的参数内容，完成系统调用对应的功能，最后把返回值放到`eax`里。**

#### `judge_pointer()`

根据Pintos文档描述

> 要实现系统调用，您需要提供在用户虚拟地址空间中读取和写入数据的方法。**在获得系统调用号之前，您需要此功能，因为系统调用号位于用户虚拟地址空间中的用户堆栈上。**这可能有点棘手：如果用户提供了无效的指针、指向内核内存的指针或其中一个区域的部分块，该怎么办？您应该通过终止用户进程来处理这些情况。**我们建议在实现`任何其他系统调用功能`之前编写和测试此代码。**有关更多信息，请参阅第3.1.5节访问用户内存。

因此，我们需要**自定义**一个判断函数，**来完成判断用户提供的指针指向的地址是否合法**。

支持读取和写入用户内存以进行系统调用。

至少有两种合理的方法可以正确地做到这一点。

第一种方法是验证用户提供的指针的有效性，然后取消引用它。如果您选择这条路线，您将需要查看 `userprog/pagedir.c`和`threads/vaddr.h`中的函数。这是处理用户内存访问的最简单方法。

第二种方法是只检查用户指针是否指向下方`PHYS_BASE`，然后取消引用它。无效的用户指针将导致“页面错误”，你可以通过修改代码的处理`page_fault()`在 `userprog / exception.c`。这种技术通常更快，因为它利用了处理器的`MMU`，所以它往往用于实际内核（包括`Linux`）。

如何判断此时指针指向的是属于用户的内存还是内核的内存呢？实验手册上给出了我们关于内存的介绍

<img src="Project 2实验手册.assets/image-20211125004412847.png" alt="image-20211125004412847" style="zoom:67%;" />

下表显示了在**用户程序开始之前**堆栈和相关寄存器的状态，假设`PHYS_BASE`为0xc000000：

在本例中，堆栈指针将初始化为0xbfffffcc。

| Address      | Name           | Data         | Type          |
| ------------ | -------------- | ------------ | ------------- |
| `0xbffffffc` | `argv[3][...]` | bar\0        | `char[4]`     |
| `0xbffffff8` | `argv[2][...]` | foo\0        | `char[4]`     |
| `0xbffffff5` | `argv[1][...]` | -l\0         | `char[3]`     |
| `0xbfffffed` | `argv[0][...]` | /bin/ls\0    | `char[8]`     |
| `0xbfffffec` | word-align     | 0            | `uint8_t`     |
| `0xbfffffe8` | `argv[4]`      | `0`          | `char *`      |
| `0xbfffffe4` | `argv[3]`      | `0xbffffffc` | `char *`      |
| `0xbfffffe0` | `argv[2]`      | `0xbffffff8` | `char *`      |
| `0xbfffffdc` | `argv[1]`      | `0xbffffff5` | `char *`      |
| `0xbfffffd8` | `argv[0]`      | `0xbfffffed` | `char *`      |
| `0xbfffffd4` | `argv`         | `0xbfffffd8` | `char **`     |
| `0xbfffffd0` | `argc`         | 4            | `int`         |
| `0xbfffffcc` | return address | 0            | `void (*) ()` |

如上所示，**您的代码应该从用户虚拟地址空间的最顶端开始堆栈**，在虚拟地址`PHYS_BASE`（在`threads/vaddr.h`中定义）下方的页面中。

因此，我们不仅需要检测指针指向的是用户存储空间还是内核存储空间，我们还需要判断当前指针是否有可能指向的是内核空间，将指针加4，如果加4后指向的地址属于内核存储空间（加4的原因是因为Type char[4]），则应该报错。

在`userprog/syscall.c`中定义函数`judge_ptr()`

**`judge_pointer()`**

```c
void * 
judge_pointer(const void *vaddr)
{ 
  void *ptr = pagedir_get_page (thread_current()->pagedir, vaddr);
  /* 判断是否属于用户地址空间，空间是否已经被映射（有效性） */
  if (!is_user_vaddr(vaddr) || ptr == NULL)
  {
    // 设置线程状态值为-1，表示线程错误
    thread_current()->st_exit = -1;
    // 线程退出
  	thread_exit ();
  }
  /* 广度范围上的判断 */
  uint8_t *check_byteptr = (uint8_t *) vaddr;
  // 注意，最多加到3，因为4的时候已经跳出了一个内存单位
  for (uint8_t i = 0; i < 4; i++) 
  {
    // 判断辅助函数
    if (get_user(check_byteptr + i) == -1)
    {
    thread_current()->st_exit = -1;
  	thread_exit ();
    }
  }
  return ptr;
}
```

> 调用的相关函数

**`is_user_vaddr()`**

```c
/* Returns true if VADDR is a user virtual address. */
static inline bool
is_user_vaddr (const void *vaddr) 
{
  return vaddr < PHYS_BASE;
}
```

返回传入的地址指针是否指向的是用户虚拟空间

判断的方法也很简单，只要指针值小于定义的物理地址范围，就属于用户地址空间，返回`false`，否则返回`true`

**`pagedir_get_page()`**

```c
/* Looks up the physical address that corresponds to user virtual
   address UADDR in PD.  Returns the kernel virtual address
   corresponding to that physical address, or a null pointer if
   UADDR is unmapped. */
void *
pagedir_get_page (uint32_t *pd, const void *uaddr) 
{
  uint32_t *pte;

  ASSERT (is_user_vaddr (uaddr));
  
  pte = lookup_page (pd, uaddr, false);
  if (pte != NULL && (*pte & PTE_P) != 0)
    return pte_get_page (*pte) + pg_ofs (uaddr);
  else
    return NULL;
}
```

查找与PD中的用户虚拟地址UADDR相对应的物理地址。返回与该物理地址对应的内核虚拟地址，如果UADDR未映射，则返回空指针

**`get_user()`**

实验手册中给了我们辅助函数的定义

<img src="Project 2实验手册.assets/image-20211125085859714.png" alt="image-20211125085859714" style="zoom:67%;" />

```c
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
```

读取用户虚拟地址UADDR处的字节。UADDR必须低于PHYS_BASE。如果成功，则返回字节值；如果发生segfault，则返回-1。

这个函数我们也需要定义在`syscall.c`文件中，因为我们需要+4来判断用户调用的指针指向的虚拟地址是否指向了内核存储空间。

#### `page_fault()`

我们不仅需要主动判断指针指向内存错误，也需要在内存错误处理函数中处理这种错误

> 这些函数中的每一个都假设用户地址已被验证为低于PHYS_BASE。他们还假设您已经修改了page_fault（），因此内核中的页面错误只会将`eax`设置为0xffffffff，并将其以前的值复制到`eip`中。

在`execption.c`文件中

```c
static void
page_fault (struct intr_frame *f) 
{
  bool not_present;  /* True: not-present page, false: writing r/o page. */
  bool write;        /* True: access was write, false: access was read. */
  bool user;         /* True: access by user, false: access by kernel. */
  void *fault_addr;  /* Fault address. */

  asm ("movl %%cr2, %0" : "=r" (fault_addr));

  intr_enable ();

  /* Count page faults. */
  page_fault_cnt++;

  /* Determine cause. */
  not_present = (f->error_code & PF_P) == 0;
  write = (f->error_code & PF_W) != 0;
  user = (f->error_code & PF_U) != 0;
    
  // 新增的代码
  // 发生错误，cpu返回-1，退出程序
  if (!user)
  {
     f->eip = f->eax;//eip:寄存器存放下一个CPU指令存放的内存地址 EAX:返回值。bshd
     f->eax = -1;
     return;
  }

  printf ("Page fault at %p: %s error %s page in %s context.\n",
          fault_addr,
          not_present ? "not present" : "rights violation",
          write ? "writing" : "reading",
          user ? "user" : "kernel");
  kill (f);
}
```

#### `syscall_init()`

准备第二步，实现系统调用的第一步，也就是获取到系统调用

系统调用属于中断，因此初始程序中的第一行其实就是利用中断机制

在文件`src/lib/syscall-nr.h`中，我们可以找到所有系统调用的标识号

```c
/* System call numbers. */
enum 
  {
    /* Projects 2 and later. */
    SYS_HALT,                   /* Halt the operating system. */
    SYS_EXIT,                   /* Terminate this process. */
    SYS_EXEC,                   /* Start another process. */
    SYS_WAIT,                   /* Wait for a child process to die. */
    SYS_CREATE,                 /* Create a file. */
    SYS_REMOVE,                 /* Delete a file. */
    SYS_OPEN,                   /* Open a file. */
    SYS_FILESIZE,               /* Obtain a file's size. */
    SYS_READ,                   /* Read from a file. */
    SYS_WRITE,                  /* Write to a file. */
    SYS_SEEK,                   /* Change position in a file. */
    SYS_TELL,                   /* Report current position in a file. */
    SYS_CLOSE,                  /* Close a file. */
}
```

> C语言`enum`
>
> 枚举类，每一项的索引值默认从0开始依次递增

怎么获取到系统调用的栈指针指向的当前值呢？

我们可以发现源程序中的`syscall_call()`的参数出现了一个结构体：`intr_frame`，我们在`user/interrupt.h`文件中找出其具体定义

```c
/* Interrupt stack frame. */
struct intr_frame
  {
    /* Pushed by intr_entry in intr-stubs.S. */
    uint16_t ss, :16;           /* Data segment for esp. */
  };
```

因此，这个结构体存储的就是当前系统调用的栈指针指向的值，所以，我们需要定义一个以此结构为类型的数组`syscallArray`，从中取出当前的指针值，根据系统调用类型的不同，执行不同的系统调用

在 Pintos 中，用户程序调用整数 $0x30进行系统调用，此时用户就会把没有权限干的活交给系统调用去干，系统调用的栈指针就是`esp`，返回值是`eax`

所以，第一行语句的函数`intr_register_int()`其实就是调用了Pintos的系统中断功能，将按照传入的值进行中断类型的分类与存储

```c
static void
register_handler (uint8_t vec_no, int dpl, enum intr_level level,
                  intr_handler_func *handler, const char *name)
{
  ASSERT (intr_handlers[vec_no] == NULL);
  if (level == INTR_ON)
    idt[vec_no] = make_trap_gate (intr_stubs[vec_no], dpl);
  else
    idt[vec_no] = make_intr_gate (intr_stubs[vec_no], dpl);
  // 存储本次系统中断需要执行的操作，我们传入的是系统中断调用的函数名
  intr_handlers[vec_no] = handler;
  // 存储本次系统中断的名称，我们传入的是syscall，表示系统调用
  intr_names[vec_no] = name;
}

void
intr_register_int (uint8_t vec_no, int dpl, enum intr_level level,
                   intr_handler_func *handler, const char *name)
{
  ASSERT (vec_no < 0x20 || vec_no > 0x2f);
  register_handler (vec_no, dpl, level, handler, name);
}
```

第一步系统调用中断存储完成后，我们就可以在数组中存储系统调用对应的操作，使得之后在执行系统调用时执行数组中对应的操作即可

```c
// src/userprog/syscall.c

// 存储系统调用类型的数组syscallArray
static void (*syscallArray[max_syscall])(struct intr_frame *);
// 系统调用
void sys_halt(struct intr_frame* f); /* syscall halt. */
void sys_exit(struct intr_frame* f); /* syscall exit. */
void sys_exec(struct intr_frame* f); /* syscall exec. */
void sys_create(struct intr_frame* f); /* syscall create */
void sys_remove(struct intr_frame* f); /* syscall remove */
void sys_open(struct intr_frame* f);/* syscall open */
void sys_wait(struct intr_frame* f); /*syscall wait */
void sys_filesize(struct intr_frame* f);/* syscall filesize */
void sys_read(struct intr_frame* f);  /* syscall read */
void sys_write(struct intr_frame* f); /* syscall write */
void sys_seek(struct intr_frame* f); /* syscall seek */
void sys_tell(struct intr_frame* f); /* syscall tell */
void sys_close(struct intr_frame* f); /* syscall close */

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
```

#### `syscall_handler()`

`syscall_handler()`函数执行系统调用函数

```c
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
```

这些工作完成后，我们就可以正式开始完成系统调用函数功能代码实现了。

#### `halt()`

