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
//#include <linux/init.h>
//#include <linux/syscalls.h>
#include <linux/kthread.h>

#define AUTHOR "Fernando Vanyo <fernando@fervagar.com>"
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

