/*
 *  linux/mykernel/myinterrupt.c
 *
 *  Kernel internal my_timer_handler
 *
 *  Copyright (C) 2013  lizhanbin
 *
 */
#include <linux/types.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/tty.h>
#include <linux/vmalloc.h>

#include "mypcb.h"

extern tPCB task[MAX_TASK_NUM];
extern tPCB * my_current_task;
extern volatile int my_need_sched;
volatile int time_count = 0;

/*
 * Called by timer interrupt.
 * it runs in the name of current running process,
 * so it use kernel stack of current running process
 */
void my_timer_handler(void)
{
#if 1
    if(time_count%1000 == 0 && my_need_sched != 1)	//my_need_sched : if 0 ,then I need to be scheduled ; if 1 ,then I don't need. 
    {
        printk(KERN_NOTICE ">>>my_timer_handler here<<<\n");
        my_need_sched = 1;
    } 
    time_count ++ ;  
#endif
    return;  	
}

void my_schedule(void)
{
    tPCB * next; //the next process   	local var
    tPCB * prev; //the current process	local var

    if(my_current_task == NULL 
        || my_current_task->next == NULL)	//if there is no current task or there is no task next ,then return.
    {
    	return;
    }
    printk(KERN_NOTICE ">>>my_schedule<<<\n");
    /* schedule */
    next = my_current_task->next; //下一个进程为当前进程的下一个进程	
    prev = my_current_task; //当前进程为变量当前进程
    if(next->state == 0)/* -1 unrunnable, 0 runnable, >0 stopped */     //当两个就绪进程切换时，要保存堆栈状态。执行到
    {                                                     //哪是不知道的，切换的点可以固定，eip始终是标号1，为什么？
	/*下一个进程可运行则进行调度*/             //主动调度，所有的进程都会调用sheclde（），切换点就在这里。
    	/* switch to next process */           //被动调度，发生中断，硬件自动保存当前到堆栈。在调用中断服务程序调用sheclde（）
	/*保存现场*/                               //中断上下文。进程切换上下文。
    	asm volatile(	
        	"pushl %%ebp\n\t" 	    /* save ebp */ //把当前的cpu的ebp加到pre的堆栈
        	"movl %%esp,%0\n\t" 	/* save esp */ //把esp的内容放到prev->thread.ip
        	"movl %2,%%esp\n\t"     /* restore  esp *///2 next的esp
        	"movl $1f,%1\n\t"       /* save eip  */ /* $1f 意思是向后查找标号1的地址 立即数*///eip放到prev->thread.ip
        	"pushl %3\n\t"       //push到当前进程堆栈
        	"ret\n\t" 	            /* restore  eip 跳转执行另一个进程，当前进程冻结*///弹出
        	"1:\t"                  /* next process start here */
        	"popl %%ebp\n\t"                                                          // 多了》？？？？？？？？？？？？？？？？
        	: "=m" (prev->thread.sp),"=m" (prev->thread.ip)
        	: "m" (next->thread.sp),"m" (next->thread.ip)
    	); 
    	my_current_task = next; 
    	printk(KERN_NOTICE ">>>switch %d to %d<<<\n",prev->pid,next->pid);   	
    }
    else
    {
        next->state = 0;     //置为0？              启动一个新进程，要给它一个执行环境，ebp esp eip（进程入口）
        my_current_task = next;
        printk(KERN_NOTICE ">>>switch %d to %d<<<\n",prev->pid,next->pid);
    	/* switch to new process */
    	asm volatile(	
        	"pushl %%ebp\n\t" 	    /* save ebp */
        	"movl %%esp,%0\n\t" 	/* save esp */
        	"movl %2,%%esp\n\t	"     /* restore  esp */
        	"movl %2,%%ebp\n\t"     /* restore  ebp */
        	"movl $1f,%1\n\t"       /* save eip */	
        	"pushl %3\n\t"     //
        	"ret\n\t" 	            /* restore  eip */
        	: "=m" (prev->thread.sp),"=m" (prev->thread.ip)
        	: "m" (next->thread.sp),"m" (next->thread.ip)
    	);          
    }   
    return;	
}

