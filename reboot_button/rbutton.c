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

/**
 *  This Kernel Module handles an interruption produced by a simple button, 
 *  connected to a GPIO. It is based on a Raspberry Pi 3, but it can be useful
 *  in other hardware. The user can set a flag through the file /proc/reboot_flag
 *  so for example:
 *      # echo 0 > /proc/reboot_flag  :: Disables the button functionality
 *      # echo 1 > /proc/reboot_flag  :: Enables the button functionality
 *  This is useful if the machine is performing some important work and
 *  the user wants to disable the button temporally.
 *
 *  In addition, it has 2 leds in order to indicate if the button is enabled or not
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>

#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/proc_fs.h>
#include <linux/reboot.h>

#define AUTHOR "Fernando Vanyo <fernando@fervagar.com>"
#define DESC   "Reboot Button Module"

MODULE_LICENSE("GPL");
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESC);

#define GPIO_GLED_PIN_N         (4)    // Green LED:    GPIO 4; PIN 7
#define GPIO_GLED_PIN_D         "Green LED for 'Reboot Button'"
#define GPIO_RLED_PIN_N         (3)    // Red LED:      GPIO 3; PIN 5
#define GPIO_RLED_PIN_D         "Red LED for 'Reboot Button'"

#define GPIO_INT_PIN_N          (2)    // Interruption: GPIO 2; PIN 3
#define GPIO_INT_PIN_D          "Reboot Button GPIO PIN"
#define GPIO_INT_DEVICE_D       (NULL)

#define PROC_RB_FILENAME       "reboot_flag"

#define REBOOT_DELAY            (2 * HZ)     // 2 seconds

static bool reboot_flag = true;
static int irq_number = 0;
static struct work_struct ws;  // For the Work Queue
static atomic_t ws_in_use = ATOMIC_INIT(0);
static struct proc_dir_entry *proc_entry;
//static struct mutex rb_mutex;

// -- Functions related with the procfs -- //
static ssize_t procfs_read(struct file *f, char __user *buf, size_t len, loff_t *off);
static ssize_t procfs_write(struct file *f, const char __user *buf, size_t len, loff_t *off);

/**
 *  NOTE: This function is quite simple... but if we want to have a good, well written procfsread function
 *  we will need to apply more logic. 
 */
static ssize_t procfs_read(struct file *f, char __user *buf, size_t len, loff_t *off) {
        static ssize_t written = 0;

        if(written) {
                return (written = 0);   // Stop reading
        }

        if((written = sprintf(buf, "%s\n", ((reboot_flag)? "Enabled" : "Disabled"))) <= 0) {
                return -EFAULT;
        }

        return written;
}

static ssize_t procfs_write(struct file *f, const char __user *buf, size_t len, loff_t *off) {
        int first_byte;

        if(len > 0) {
                first_byte = (int) *buf;
                reboot_flag = ('1' == ((char) first_byte));
        }

        return len;
}

static const struct file_operations pfops = {
        write: (*procfs_write),
        read: (*procfs_read),
};

static void set_red_led(bool state) {
        if(gpio_direction_output(GPIO_RLED_PIN_N, state) < 0) {
               printk(KERN_ERR "Error setting %d to the red LED\n", state);
        }
}

static void set_green_led(bool state) {
        if(gpio_direction_output(GPIO_GLED_PIN_N, state) < 0) {
               printk(KERN_ERR "Error setting %d to the green LED\n", state);
        }
}

static void rb_disable_red_led(struct work_struct *work) {
        struct delayed_work *dw;

        dw = container_of(work, struct delayed_work, work);

        // Disable red LED
        set_red_led(false);

        kfree(dw);

        // Set the ws as free
        atomic_dec(&ws_in_use);
}

static void rb_perform_reboot(struct work_struct *work) {
        struct delayed_work *dw;

        dw = container_of(work, struct delayed_work, work);

        // Disable green LED
        set_green_led(false);

        //kernel_restart(NULL);
        orderly_reboot();

        kfree(dw);              // Well, actually this is never reached
        atomic_dec(&ws_in_use); // Neither decrementing the 'ws_in_use'
}

static void process_context_function(struct work_struct *work) {
        struct delayed_work *dw;

        dw = kmalloc(sizeof(*dw), GFP_KERNEL);

        if(reboot_flag) {
                printk(KERN_EMERG "'Reboot button' pressed. Rebooting the system!!\n");
                // Enable green LED
                set_green_led(true);

                INIT_DELAYED_WORK(dw, rb_perform_reboot);
        }
        else {
                // Enable red LED
                set_red_led(true);

                INIT_DELAYED_WORK(dw, rb_disable_red_led);
        }

        schedule_delayed_work(dw, REBOOT_DELAY);
}

// -- Interruption Handler -- //
static irqreturn_t rbutton_handler(int irq, void *dev_id, struct pt_regs *regs) {
        //static spinlock_t interrupt_lock = SPIN_LOCK_UNLOCK; // Deprecated
        static DEFINE_SPINLOCK(interrupt_lock);
        unsigned long flags;

        spin_lock_irqsave(&interrupt_lock, flags);

        if(!atomic_read(&ws_in_use)) {
                // Set the flag
                atomic_inc(&ws_in_use);

                spin_unlock_irqrestore(&interrupt_lock, flags);
                
                // Initialize the work for the Work Queues
                INIT_WORK(&ws, process_context_function);

                schedule_work(&ws);
        }
        else {
                spin_unlock_irqrestore(&interrupt_lock, flags);
        }

        return IRQ_HANDLED;
}

static inline int request_gpio_pin(unsigned int gpio, const char *label, bool input_mode) {
        // Try to allocate the GPIO
        if(gpio_request(gpio, label)) {
                printk(KERN_ERR "Error requesting GPIO %d\n", gpio);
                return -1;
        }

        if(input_mode) {
                if(gpio_direction_input(gpio) < 0) {
                        printk(KERN_ERR "Error setting GPIO %d as INPUT\n", gpio);
                        gpio_free(gpio);
                        return -1;
                }
        }

        return 0;
}

// -- Module functions -- //
static int __init module_entry_point(void){
        // Request Interruption GPIO
        if(request_gpio_pin(GPIO_INT_PIN_N, GPIO_INT_PIN_D, true) < 0) {
                return -1;
        }

        // Try the IRQ Mapping (Interrupt Request Line)
        if((irq_number = gpio_to_irq(GPIO_INT_PIN_N)) < 0) {
                printk(KERN_ERR "Error mapping IRQ (GPIO %d)\n", GPIO_INT_PIN_N);
                gpio_free(GPIO_INT_PIN_N);
                return -1;
        }

        printk(KERN_INFO "IRQ for 'Reboot Button' (GPIO %d) mapped into line %d\n", GPIO_INT_PIN_N, irq_number);

        // Request this IRQ Handler to the Kernel
        if(request_irq(irq_number, (irq_handler_t) rbutton_handler, IRQF_TRIGGER_FALLING,
                                                        GPIO_INT_PIN_D, GPIO_INT_DEVICE_D)) {
                printk(KERN_ERR "Error requesting irq %d to the kernel\n", irq_number);
                gpio_free(GPIO_INT_PIN_N);
                return -1;
        }

        // Add an entry in /proc/
        proc_entry = proc_create(PROC_RB_FILENAME, 0, NULL, &pfops);

        // Setup the leds //

        // Request Green LED GPIO
        if(request_gpio_pin(GPIO_GLED_PIN_N, GPIO_GLED_PIN_D, false) < 0) {
                return -1;
        }

        // Request Red LED GPIO
        if(request_gpio_pin(GPIO_RLED_PIN_N, GPIO_RLED_PIN_D, false) < 0) {
                return -1;
        }

        // Disable leds (just in case the GPIO pin is HIGH)
        set_green_led(false);
        set_red_led(false);

        return 0;
}

static void __exit module_exit_point(void) {
        // Release the irq
        free_irq(irq_number, GPIO_INT_DEVICE_D);

        // Free GPIO resources
        gpio_free(GPIO_INT_PIN_N);
        gpio_free(GPIO_GLED_PIN_N);
        gpio_free(GPIO_RLED_PIN_N);

        // Free the /proc/ file
        if(proc_entry) {
                remove_proc_entry(PROC_RB_FILENAME, NULL);
        }

        // Just in case:
        flush_work(&ws);
}

module_init(module_entry_point);
module_exit(module_exit_point);

