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

#define AUTHOR "Fernando Vanyo <fernando@fervagar.com>"
#define DESC   "Module that show the generation of a process"

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESC);

int pid = 0;
module_param(pid, int, S_IRUGO);

asmlinkage unsigned long generation(int argpid);

EXPORT_SYMBOL(generation);

asmlinkage unsigned long generation(int argpid){
	int generation = 1;
	struct task_struct *p;

	for_each_process(p)
		if (p->pid == argpid) break;

	for (; p->pid != 1; generation++) p = p -> parent;
	
	return generation;
}

static int __init entry_point(void) {
	if(pid)
		printk(KERN_DEBUG "PID: %d - Generation: %ld\n", pid, generation(pid));

	return 0;
}

static void __exit exit_point(void) {
	return;
}

module_init(entry_point);
module_exit(exit_point);

