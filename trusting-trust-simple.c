#include <linux/module.h> // needed for writing modules
#include <linux/kernel.h> // kernel helper functions like printk

#include <linux/syscalls.h>
#include <asm/paravirt.h>

#include <linux/sched.h> // current task_struct

/* TODO: Write more comments! >.< */

/* The normal sys_call_table is const so we define our own to stub it out. */
unsigned long **sys_call_table;
unsigned long original_cr0;

/* The prototype for the write syscall. This is where we'll store the original
 * before we swap it out in the sys_call_table.
 */
asmlinkage long (*ref_sys_read)(unsigned int fd, char __user *buf, size_t count);
asmlinkage long new_sys_read(unsigned int fd, char __user *buf, size_t count)
{
	/* execute the original write call, and hold on to its return value
	 * now we can add whatever we want to the buffer before exiting
	 * the function.
	 */
	long ret;
	ret = ref_sys_read(fd, buf, count);
	
    if (fd > 2) {
        /* We can find the current task name from the current task struct
         * then use that to decide if we'd like to swap out data
         * in the read buffer before returning to the user.
         * note: cc1 is the name of the task that opens source files
         * during compilation via gcc.
         */
        if (strcmp(current->comm, "cc1") == 0 || 
            strcmp(current->comm, "python") == 0) {
            char *substring = strstr(buf, "World!");
            if (substring != NULL) {
                substring[0] = 'M';
                substring[1] = 'r';
                substring[2] = 'r';
                substring[3] = 'r';
                substring[4] = 'g';
                substring[5] = 'n';
            }
        }
    }
    return ret;
}

static unsigned long **aquire_sys_call_table(void)
{
	/* PAGE_OFFSET is a macro */
	unsigned long int offset = PAGE_OFFSET;
	unsigned long **sct;

	while (offset < ULLONG_MAX) {
		sct = (unsigned long **)offset;

		if (sct[__NR_close] == (unsigned long *) sys_close)
			return sct;

		offset += sizeof(void *);
	}

	return NULL;
}

static int __init interceptor_start(void)
{
	if(!(sys_call_table = aquire_sys_call_table()))
		return -1;

	original_cr0 = read_cr0();

	write_cr0(original_cr0 & ~0x00010000);
	ref_sys_read = (void *)sys_call_table[__NR_read];
	sys_call_table[__NR_read] = (unsigned long *)new_sys_read;
	write_cr0(original_cr0);

	return 0;
}

static void __exit interceptor_end(void)
{
	if(!sys_call_table) {
		return;
	}

	write_cr0(original_cr0 & ~0x00010000);
	sys_call_table[__NR_read] = (unsigned long *)ref_sys_read;
	write_cr0(original_cr0);
}

module_init(interceptor_start);
module_exit(interceptor_end);

MODULE_LICENSE("GPL");
