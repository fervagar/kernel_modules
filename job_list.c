#ifndef __KERNEL__
#  define __KERNEL__
#endif
#ifndef MODULE
#  define MODULE
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/syscalls.h>
#include <linux/kthread.h>
#include <linux/slab.h>

#define AUTHOR "Fernando Vanyo <fervagar@tuta.io>"
#define DESC   "Simple Job queue for a unique kernel thread"

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESC);

#define CHECK_EXIT					\
	do{						\
		set_current_state(TASK_INTERRUPTIBLE);	\
		if (kthread_should_stop()){		\
			break;				\
		}					\
	}while(0)

// List of Jobs for the worker thread //
typedef struct job_t {
	void* (*func)(void* args);
	void* args;
	struct list_head list;
} job_t;


struct task_struct *task; // the worker kthread
job_t jobs;

void generic_job(void);

static int main_thread(void *data){
	void* (*func_ptr)(void* arg);
	void* args;
	job_t *job_ptr;

	while(!kthread_should_stop()){
		//do work
		if(!list_empty(&jobs.list)){
			CHECK_EXIT;
			printk(KERN_DEBUG "[kernel thread] Oh... I have job! :)\n");

			// Get a job
			job_ptr = list_first_entry_or_null(&jobs.list, struct job_t, list);
			if (job_ptr != NULL){
				printk(KERN_DEBUG "[kernel thread] doing the job....\n");
				func_ptr = job_ptr->func;
				args = job_ptr->args;
				func_ptr(args);
				// Free the node
				list_del(&job_ptr->list);
				kfree(job_ptr);
			}
		}
		else{
			//printk(KERN_DEBUG "[kernel thread] List empty....\n");
			// Wait 500 ms //
			CHECK_EXIT;
			schedule_timeout(0.5 * HZ);
		}
	}

	printk(KERN_DEBUG "[kernel thread] bye!!\n");
	return 0;
}

void generic_job(){
	printk(KERN_INFO "This is the generic job! I am %s\n", current->comm);
	
	return;
}

static int __init entry_point(void) {
	job_t *job_ptr;
	
	INIT_LIST_HEAD(&jobs.list);

	task = kthread_create(&main_thread, NULL, "MyKernelThread");
	wake_up_process(task);

	// wait 2 seconds
	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(2 * HZ); //Wait 2 seconds
	// add a job to the queue
	printk(KERN_DEBUG "[%s] OK. 2 seconds later, I add a job to the queue\n", current->comm);


	job_ptr = (job_t *) kmalloc(sizeof(struct job_t), GFP_KERNEL);
	if(job_ptr != NULL){
		job_ptr->func = (void*)generic_job;
		job_ptr->args = NULL;
	}
	// else -> error in kmalloc :S

	list_add_tail(&job_ptr->list, &jobs.list);

	return 0;
}

static void __exit exit_point(void) {
	printk(KERN_DEBUG "[%s] bye!!\n", current->comm);
	kthread_stop(task);

	return;
}

module_init(entry_point);
module_exit(exit_point);

