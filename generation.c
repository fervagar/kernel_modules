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

#define AUTHOR "Fernando Vanyo <fervagar@tuta.io>"
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

