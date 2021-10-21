# PintosOS Project1 说明文档

> **19231224 卢恒润**

## PintosOS文件目录结构

Pintos是美国斯坦福大学操作系统课程的实验项目，作为一个小型操作系统，Pintos实现了进程管理，多线程并发控制，文件系统，虚拟内存等功能，但是这些功能还并不完善，你需要做的就是对这个文件进行相应的代码添加，修改或者删除，如果您能通过所有的测试点，则实验成功。

Pintos文件中我们主要关心src文件夹的目录结构，如下图所示：

![image-20211021083404912](C:\Users\lhr4108\AppData\Roaming\Typora\typora-user-images\image-20211021083404912.png)

| 目录        | 主要负责功能                                                 |
| ----------- | :----------------------------------------------------------- |
| devices/    | I/O设备管理，其中timer.c文件需要在**Project1**中进行修改     |
| examples/   | 从**Project2**开始使用的示例用户程序                         |
| filesys/    | 基本文件系统的源代码，您将从**Project 2**开始使用此文件系统，但在**Project 4**之前不会对其进行修改 |
| lib/        | 标准C库的一个子集的实现。此目录中的代码被编译到Pintos内核中，并且从**Project 2**开始，编译到在其下运行的用户程序中。在内核代码和用户程序中，可以使用`#include<…>`符号包含此目录中的头。**您应该不需要修改此代码** |
| lib/kernel/ | C库中仅包含在Pintos内核中的部分。这还包括一些可以在内核代码中自由使用的数据类型的实现：位图、双链接列表和哈希表。在内核中，可以使用`#include<…>`符号包含此目录中的头 |
| lib/user/   | C库中仅包含在Pintos用户程序中的部分。在用户程序中，可以使用`#include<…>`符号包含此目录中的标题 |
| misc/       | 如果您决定在自己的机器上尝试使用Pintos，这些文件可能会派上用场。否则，**您可以忽略它们** |
| utils/      | 如果您决定在自己的机器上尝试使用Pintos，这些文件可能会派上用场。否则，**您可以忽略它们** |
| tests/      | 每个项目的测试代码，如果此代码有助于测试您的提交，您可以修改此代码，但在运行测试之前，我们会将其替换为原始代码 |
| threads/    | 基本内核的源代码，您将从**Project 1**开始对其进行修改        |
| userprog/   | 用户程序加载器的源代码，您将从**Project 2**开始修改它        |
| vm/         | 几乎是空的目录，您将在**Project 3**中实现虚拟内存            |

参考网址：http://web.stanford.edu/~ouster/cgi-bin/cs140-spring20/pintos/pintos_1.html

对于第一次实验，我们需要修改的文件全部集中在`src/threads`目录和`src/devices`目录之下。

## Pintos Debug Tools

您可以使用许多工具调试Pintos，本节内容向您介绍了其中的一些

### 1 printf()方法

不要低估`printf()`的作用，`printf()`在Pintos中的实现方式是，您几乎可以从内核中的任何位置调用它，无论它是在内核线程中还是在中断处理程序中，几乎不管持有什么锁。



### 2 ASSERT断言

断言是有用的，因为它们可以在问题被发现之前及早发现问题。理想情况下，每个函数应该以一组断言开始，这些断言检查其参数的有效性。（函数的局部变量的初始值设定项在检查断言之前进行评估，因此小心不要假设初始值设定项中的参数有效。）您还可以将断言散布到整个函数体中怀疑可能出错的地方。它们对于检查循环不变量特别有用。
Pintos提供了在`<debug.h>`中定义的ASSERT宏，用于检查断言。

```c
if (CONDITION) {} 
else {
    PANIC ("assertion `%s' failed.", #CONDITION);
}
```

测试表达式的值。如果它的计算结果为零（false），那么内核将陷入恐慌(panic)。紧急消息包括失败的表达式、其文件和行号以及回溯，这将帮助您找到问题。



### 3 宏定义参数

`<debug.h>`中定义的这些宏告诉编译器函数或函数参数的特殊属性。它们的扩展是特定于GCC的。

- **UNUSED**

未使用

附加到函数参数后，告知编译器该参数可能不在函数中使用,它将抑制否则会出现的警告。

- **NO_RETURN**

无返回值

附加到函数原型中，告诉编译器函数永远不会返回。它允许编译器微调其警告和代码生成。

- **NO_INLINE**

无内联

附加到函数原型中，告诉编译器永远不要在线发出函数。有时对提高回溯的质量很有用

- **PRINTF_FORMAT**(format,first)

附加到函数原型后，告诉编译器函数采用类似`printf()`的格式字符串作为参数编号格式（从1开始），并且相应的值参数从第一个编号的参数开始。这让编译器告诉您是否传递了错误的参数类型。



### 4 GDB调试

您可以在GDB调试器的监督下运行Pintos

1 使用`--gdb`选项启动Pintos

注意，终端位置：`src/threads/build`

```shell
pintos --gdb -- run mytest
```

完成这一步后，bochs打开终端调试模式

![image-20211021102944325](C:\Users\lhr4108\AppData\Roaming\Typora\typora-user-images\image-20211021102944325.png)

2 在同一台机器上打开第二个终端，并使用`pintos gdb`调用kernel.o上的`gdb`

```shell
pintos-gdb kernel.o
```

3 输入以下命令

```shell
(gdb) target remote localhost:1234
```

现在GDB通过本地网络连接到模拟器。您现在可以发出任何普通的GDB命令。如果发出`c`命令，模拟BIOS将控制、加载Pintos，然后Pintos将以通常的方式运行。您可以使用Ctrl+C在任意点暂停进程。

> **ps**：按照教程是这么走的，但是我的电脑到第二步后输入指令之后没反应，我也不知道是怎么回事，但利用gdb调试这条路应该走不通了
>
> ![image-20211021105413121](C:\Users\lhr4108\AppData\Roaming\Typora\typora-user-images\image-20211021105413121.png)
>
> 正常情况应该是这样的：
>
> ![image-20211021105710812](C:\Users\lhr4108\AppData\Roaming\Typora\typora-user-images\image-20211021105710812.png)

参考网址：https://web.stanford.edu/class/cs140/projects/pintos/pintos_10.html





在本作业中，我们将为您提供一个功能最小的线程系统。您的工作是扩展此系统的功能，以便更好地了解同步问题。

您将主要在`threads`目录中进行此分配，同时在`devices`目录中进行一些工作。编译应该在`threads`目录中完成。

## threads部分源代码

您不需要修改大部分代码，但希望通过介绍此概述，您可以开始了解要查看的代码。

| 文件名                        | 文件作用                                                     |
| ----------------------------- | ------------------------------------------------------------ |
| `loader.S` `loader.h`         | 内核加载程序，**您不需要查看或修改此代码**                   |
| `start.S`                     | 在80x86 CPU上执行内存保护和32位操作所需的基本设置。与加载程序不同，此代码实际上是内核的一部分，**您不需要查看或修改此代码** |
| `kernel.lds.S`                | 用于链接内核的链接器脚本。设置内核的加载地址，并将`start.S`安排在内核映像的开头附近。同样，**您不需要查看或修改此代码** |
| `init.c` `init.h`             | 内核初始化，包括`main()`，内核的“主程序”，您至少应该查看`main()`以查看初始化的内容，您可能希望在此处添加自己的初始化代码 |
| `thread.c` `thread.h`         | 基本线程支持，您的大部分工作将在这些文件中进行，`h`定义了`struct-thread`，您可能会在所有四个项目中修改它 |
| `switch.S` `switch.h`         | 用于切换线程的汇编语言例程                                   |
| `palloc.c` `palloc.h`         | 页分配器，以4KB页的倍数分配系统内存                          |
| `malloc.c` `malloc.h`         | 内核的`malloc()`和`free()`的简单实现                         |
| `interrupt.c` `interrupt.h`   | 基本中断处理和用于打开和关闭中断的功能                       |
| `intr-stubs.S` `intr-stubs.h` | 用于低级中断处理的汇编代码                                   |
| `synch.c` `synch.h`           | 基本同步原语：信号量、锁、条件变量和优化障碍，**您将需要在所有四个项目中使用这些同步** |
| `io.h`                        | 用于I/O端口访问的功能。这主要由设备目录中的源代码使用，**您无需触摸这些源代码** |
| `vaddr.h` `pte.h`             | 用于处理虚拟地址和页表项的函数和宏，**在项目3中，这些将对您更加重要，现在，你可以忽略它们** |
| `flags.h`                     | 在80x86“标志”寄存器中定义一些位的宏                          |
| `fixed-point.h`               | 实现定点算法的函数，用于实现高级调度程序                     |

## devices部分源代码

| 文件名                      | 文件作用                                                     |
| --------------------------- | ------------------------------------------------------------ |
| `timer.c` `timer.h`         | 系统计时器，默认情况下每秒滴答100次，**您将在此项目中修改此代码** |
| `vga.c` `vga.h`             | VGA显示驱动程序，负责将文本写入屏幕，**您应该不需要查看此代码**，`printf()`为您调用VGA显示驱动程序，因此没有理由自己调用此代码 |
| `serial.c` `serial.h`       | 串口驱动程序，同样，`printf()`为您调用此代码，**因此您不需要自己这样做** |
| `block.c` `block.h`         | 块设备的抽象层，即随机访问、磁盘状设备，它们被组织为固定大小块的阵列，开箱即用的Pintos支持两种类型的块设备：IDE磁盘和分区，块设备，无论类型如何，**在项目2之前都不会实际使用** |
| `ide.c` `ide.h`             | 最多支持4个IDE磁盘上的读写扇区                               |
| `partition.c` `partition.h` | 了解磁盘上分区的结构，允许将单个磁盘划分为多个区域（分区）以供独立使用 |
| `kbd.c` `kbd.h`             | 键盘驱动程序，处理将其传递到输入层的击键                     |
| `input.c` `input.h`         | 输入层，对键盘或串行驱动程序传递的输入字符进行排队           |
| `intq.c` `intq.h`           | 中断队列，用于管理内核线程和中断处理程序都希望访问的循环队列，由键盘和串行驱动程序使用 |
| `rtc.c` `rtc.h`             | 实时时钟驱动程序，使内核能够确定当前日期和时间，默认情况下，这仅由`thread/init.c`用于为随机数生成器选择初始种子 |
| `speaker.c` `speaker.h`     | 可在PC扬声器上产生铃声的驱动程序                             |
| `pit.c` `pit.h`             | 配置8254可编程中断定时器的代码，此代码由`devices/timer.c`和`devices/speaker.c`使用，因为每个设备都使用PIT的一个输出通道 |

## lib部分源代码

最后，`lib`和`lib/kernel`包含有用的库例程，从**Project 2**开始，用户程序将使用`lib/user`，但它不是内核的一部分

| 文件名                                                       | 文件功能                                                     |
| ------------------------------------------------------------ | ------------------------------------------------------------ |
| `ctype.h` `inttypes.h` `limits.h` `stdarg.h` `stdbool.h` `stddef.h` `stdint.h` `stdio.c` `stdio.h` `stdlib.c` `stdlib.h` `string.c` `string.h` | Pintos内核引入的C语言标准库函数                              |
| `debug.c` `debug.h`                                          | 用于帮助调试的函数和宏                                       |
| `random.c` `random.h`                                        | 伪随机数发生器，随机值的实际顺序不会随Pintos的运行而变化     |
| `round.h`                                                    | 用于舍入的宏                                                 |
| `syscall-nr.h`                                               | 系统呼叫号码，直到**Project 2**才使用                        |
| `kernel/list.c` `kernel/list.h`                              | 双链表实现，在Pintos代码中使用过，**您可能希望自己在Project 1中的一些地方使用它** |
| `kernel/bitmap.c` `kernel/bitmap.h`                          | 位图实现，如果愿意，可以在代码中使用它，**但在Project 1中可能不需要它** |
| `kernel/hash.c` `kernel/hash.h`                              | 哈希表实现，**可能对Project 3有用**                          |
| `kernel/console.c` `kernel/console.h` `kernel/stdio.h`       | 实现`printf()`和其他一些函数                                 |

上述源码部分参考网址：http://web.stanford.edu/~ouster/cgi-bin/cs140-spring20/pintos/pintos_2.html

## 任务一	Alarm Clock

### 任务要求

重新实现定义在文件`devices/timer.c`中的函数**`time_sleep()`**

虽然提供了一个工作的实现，但它“忙着等待”，也就是说，它在一个循环中旋转，检查当前时间并调用线程`_yield()`，直到足够的时间过去。

**重新实现它以避免繁忙的等待。**

### 实现思路

> `time_sleep()`函数源码

```c
/*
	timer_sleep函数在devices/timer.c。系统现在是使用busy wait实现的，即线程不停地循环，直到时间片耗尽。更改timer_sleep的实现方式。
*/

void timer_sleep (int64_t ticks) 
{
  int64_t start = timer_ticks ();

  ASSERT (intr_get_level () == INTR_ON);
  while (timer_elapsed (start) < ticks) 
    thread_yield ();
}
```

