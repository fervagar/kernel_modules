#ifndef __KERNEL__
#  define __KERNEL__
#endif
#ifndef MODULE
#  define MODULE
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#define AUTHOR "Fernando Vanyo <fervagar@tuta.io>"
#define DESC   "Simple example of kernel Work Queues (using system kworkers)"

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESC);

struct work_cont {
	struct work_struct real_work;
	int    arg;
} work_cont;

static void thread_function(struct work_struct *work);

struct work_cont *test_wq;

static void thread_function(struct work_struct *work_arg){
	struct work_cont *c_ptr = container_of(work_arg, struct work_cont, real_work);

	printk(KERN_INFO "[Deferred work]=> PID: %d; NAME: %s\n", current->pid, current->comm);
	printk(KERN_INFO "[Deferred work]=> I am going to sleep 2 seconds\n");
	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(2 * HZ); //Wait 2 seconds
	
	printk(KERN_INFO "[Deferred work]=> DONE. BTW the data is: %d\n", c_ptr->arg);

	return;
}

static int __init entry_point(void) {

	test_wq = kmalloc(sizeof(*test_wq), GFP_KERNEL);
	INIT_WORK(&test_wq->real_work, thread_function);
	test_wq->arg = 31337;

	schedule_work(&test_wq->real_work);

	return 0;
}

static void __exit exit_point(void) {
	//just in case:
	flush_work(&test_wq->real_work);

	kfree(test_wq);
	return;
}

module_init(entry_point);
module_exit(exit_point);

