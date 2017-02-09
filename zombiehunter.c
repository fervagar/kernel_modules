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
#include <linux/syscalls.h>
#include <linux/kthread.h>
#include <asm/siginfo.h>

#define AUTHOR "Fernando Vanyo <fernando@fervagar.com>"
#define DESC   "Search and KILL all the zombie processes"

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESC);

struct task_struct *task;

static int hunt_zombies(void *data){
	struct task_struct *p;
	struct siginfo info;
	long remaining;
	int ret = -ESRCH;

	while(!kthread_should_stop()){
		for_each_process(p){
			if(p->state == TASK_DEAD){
				printk(KERN_INFO "Zombie Process Targeted => PID: %d; NAME: %s\n", p->pid, p->comm);
				
				/**
				 *  This is only a POC. Actually, sending a SIGKILL to a zombie process
				 *  is useless... ;)
				 */
				if ( (ret = send_sig_info(SIGKILL, &info, p)) == 0){
					printk(KERN_INFO "Process %d killed :P\n", p->pid);
				}
			}
		}

		set_current_state(TASK_INTERRUPTIBLE);
		if (kthread_should_stop()){
			break;
		}
		remaining = schedule_timeout(2 * HZ); //Wait 2 seconds
		/* as we set the thread as TASK_INTERRUPTIBLE, the routine may return early if a signal \
		is delivered to the current task. In this case the remaining time in jiffies will \
		be returned, or 0 if the timer expired in time
		*/
	}
	
	printk(KERN_DEBUG "[kernel thread] bye!!\n");	
	return 0;
}

static int __init entry_point(void) {
	task = kthread_create(&hunt_zombies, NULL, "MyKernelThread");
	wake_up_process(task);

	return 0;
}

static void __exit exit_point(void) {
	printk(KERN_DEBUG "[rmmod] bye!!\n");
	kthread_stop(task);
	return;
}

module_init(entry_point);
module_exit(exit_point);

