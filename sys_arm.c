/*
 *  linux/arch/arm/kernel/sys_arm.c
 *
 *  Copyright (C) People who wrote linux/arch/i386/kernel/sys_i386.c
 *  Copyright (C) 1995, 1996 Russell King.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  This file contains various random system calls that
 *  have a non-standard calling sequence on the Linux/arm
 *  platform.
 */
#include <linux/export.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/sem.h>
#include <linux/msg.h>
#include <linux/shm.h>
#include <linux/stat.h>
#include <linux/syscalls.h>
#include <linux/mman.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/ipc.h>
#include <linux/uaccess.h>
#include <linux/slab.h>


struct mmLimit{
	uid_t *uids;
	unsigned long *mm_maxs;
    int count;
};

struct mmLimit my_mm_limits;
EXPORT_SYMBOL(my_mm_limits);

asmlinkage long sys_MyLimit(uid_t uid, unsigned long mm_max)
{
    int i = 0;
    int ptr = -1;   
    //If the list is empty create a new one and add the new entry,
    if(my_mm_limits.uids == NULL)
    {
        my_mm_limits.uids = kmalloc_array(1000,sizeof(uid_t),GFP_KERNEL);
        my_mm_limits.mm_maxs = kmalloc_array(1000,sizeof(unsigned long),GFP_KERNEL);
        my_mm_limits.count =0;
        if(my_mm_limits.uids == NULL || my_mm_limits.mm_maxs == NULL)
        {
            printk("Allocation failed");
            return -EADDRNOTAVAIL;
        }
        my_mm_limits.uids[my_mm_limits.count] = uid;
        my_mm_limits.mm_maxs[my_mm_limits.count] = mm_max;
        my_mm_limits.count +=1;
    }    
    else
    {
        //Check whether the uid exists, if so update, else add new and add to count
        i = 0;
        ptr = -1;
        while(i < my_mm_limits.count)
        {
            if(my_mm_limits.uids[i] == uid)
            {
                ptr = i;                
                break;
            }
            i++;
        }
        if(ptr == -1)
        {
            my_mm_limits.uids[my_mm_limits.count] =uid;
            my_mm_limits.mm_maxs[my_mm_limits.count] =mm_max;
            my_mm_limits.count +=1;
        }
        else
        {
            my_mm_limits.mm_maxs[ptr] = mm_max;
        }
    }
    
	i = 0;
	printk("\n\nLIST OF MM LIMITS:\n");
	while(i < my_mm_limits.count)
	{
		printk("uid = %d, mm_max = %d\n", my_mm_limits.uids[i],my_mm_limits.mm_maxs[i]);
		i++;
	}
	printk("\n\n");
	return 0;
}

/* Fork a new task - this creates a new program thread.
 * This is called indirectly via a small wrapper
 */
asmlinkage int sys_fork(struct pt_regs *regs)
{
#ifdef CONFIG_MMU
	return do_fork(SIGCHLD, regs->ARM_sp, regs, 0, NULL, NULL);
#else
	/* can not support in nommu mode */
	return(-EINVAL);
#endif
}

/* Clone a task - this clones the calling program thread.
 * This is called indirectly via a small wrapper
 */
asmlinkage int sys_clone(unsigned long clone_flags, unsigned long newsp,
			 int __user *parent_tidptr, int tls_val,
			 int __user *child_tidptr, struct pt_regs *regs)
{
	if (!newsp)
		newsp = regs->ARM_sp;

	return do_fork(clone_flags, newsp, regs, 0, parent_tidptr, child_tidptr);
}

asmlinkage int sys_vfork(struct pt_regs *regs)
{
	return do_fork(CLONE_VFORK | CLONE_VM | SIGCHLD, regs->ARM_sp, regs, 0, NULL, NULL);
}

/* sys_execve() executes a new program.
 * This is called indirectly via a small wrapper
 */
asmlinkage int sys_execve(const char __user *filenamei,
			  const char __user *const __user *argv,
			  const char __user *const __user *envp, struct pt_regs *regs)
{
	int error;
	char * filename;

	filename = getname(filenamei);
	error = PTR_ERR(filename);
	if (IS_ERR(filename))
		goto out;
	error = do_execve(filename, argv, envp, regs);
	putname(filename);
out:
	return error;
}

int kernel_execve(const char *filename,
		  const char *const argv[],
		  const char *const envp[])
{
	struct pt_regs regs;
	int ret;

	memset(&regs, 0, sizeof(struct pt_regs));
	ret = do_execve(filename,
			(const char __user *const __user *)argv,
			(const char __user *const __user *)envp, &regs);
	if (ret < 0)
		goto out;

	/*
	 * Save argc to the register structure for userspace.
	 */
	regs.ARM_r0 = ret;

	/*
	 * We were successful.  We won't be returning to our caller, but
	 * instead to user space by manipulating the kernel stack.
	 */
	asm(	"add	r0, %0, %1\n\t"
		"mov	r1, %2\n\t"
		"mov	r2, %3\n\t"
		"bl	memmove\n\t"	/* copy regs to top of stack */
		"mov	r8, #0\n\t"	/* not a syscall */
		"mov	r9, %0\n\t"	/* thread structure */
		"mov	sp, r0\n\t"	/* reposition stack pointer */
		"b	ret_to_user"
		:
		: "r" (current_thread_info()),
		  "Ir" (THREAD_START_SP - sizeof(regs)),
		  "r" (&regs),
		  "Ir" (sizeof(regs))
		: "r0", "r1", "r2", "r3", "r8", "r9", "ip", "lr", "memory");

 out:
	return ret;
}
EXPORT_SYMBOL(kernel_execve);

/*
 * Since loff_t is a 64 bit type we avoid a lot of ABI hassle
 * with a different argument ordering.
 */
asmlinkage long sys_arm_fadvise64_64(int fd, int advice,
				     loff_t offset, loff_t len)
{
	return sys_fadvise64_64(fd, offset, len, advice);
}
