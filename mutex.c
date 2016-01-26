#ifndef __KERNEL__
#  define __KERNEL__
#endif
#ifndef MODULE
#  define MODULE
#endif

#include <linux/module.h>
#include <linux/kernel.h>
//#include <linux/init.h>
//#include <linux/syscalls.h>
#include <linux/kthread.h>

#define AUTHOR "Fernando Vanyo <fervagar@tuta.io>"
#define DESC   "Simple \"Hello World\" of mutex in kernel"

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESC);

struct task_struct *task1;
struct task_struct *task2;

static DEFINE_MUTEX(mutex);

static int i = 0;

static int hello_kthread(void *data){
	printk(KERN_DEBUG "%s entered.\n", current->comm);
	
	mutex_lock(&mutex);

	printk(KERN_DEBUG "[%s] mutex locked.\n", current->comm);
	while(i < 10){
		printk(KERN_DEBUG "[%s] i = %d.\n", current->comm, i++);
		schedule_timeout(0.2 * HZ); //Wait 200 ms
	}
	
	printk(KERN_DEBUG "[%s] unlocking mutex.\n", current->comm);
	mutex_unlock(&mutex);
	
	while(!kthread_should_stop()){
		schedule();
		//!\\ Without setting interruptible, it uses a lot of cpu... bad idea for general purposes
		set_current_state(TASK_INTERRUPTIBLE);
	}

	return 0;
}

static int __init entry_point(void) {
	task1 = kthread_create(&hello_kthread, NULL, "Thread 1");
	task2 = kthread_create(&hello_kthread, NULL, "Thread 2");
	wake_up_process(task1);
	wake_up_process(task2);

	return 0;
}

static void __exit exit_point(void) {
	kthread_stop(task1);
	kthread_stop(task2);
	return;
}

module_init(entry_point);
module_exit(exit_point);

