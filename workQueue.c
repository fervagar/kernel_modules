#ifndef __KERNEL__
#  define __KERNEL__
#endif
#ifndef MODULE
#  define MODULE
#endif

/*
 * Copyright (C) 2016 Fernando Vanyo Garcia
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *	Fernando Vanyo Garcia <fernando@fervagar.com>
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#define AUTHOR "Fernando Vanyo <fernando@fervagar.com>"
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

