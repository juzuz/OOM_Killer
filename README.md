# OOM_Killer
A system may become out-of-memory (OOM) when a buggy or malicious process uses
a lot of physical memory or when the system has too many processes running. To avoid
crashing the whole system including all processes, Linux implements an OOM killer to
selectively kill some processes and reclaim the physical memory allocated to these
processes. The OOM killer selects processes to kill using a variety of heuristics, typically
targeting the process that 1) uses a large amount of physical memory and 2) is not
important.
Although this OOM killer works reasonably well, it has one security flaw: it ignores which
users created the processes. A malicious user can thus game the OOM killer by creating
a large number of processes. While the aggregated amount of memory allocated to this
user's processes is huge, each process owns only a small amount of memory, and the
OOM killer may not notice these processes. When a user runs, it can
create more than one process. Thus, a malicious user can create a large number of
processes, causing other users’ processes be killed.
This project targets to modify the preexisting OOM killer to make it "user aware".
Each user will be limited to a predetermined amount of memory and once it runs out of memory,
the oom killer will activate and kill the "worst" process.

## Setting up the project
**1. Compile the Linux kernel.**

**a. Make sure that you have added the following path into your environment variable.**

><span style="color:blue">ANDROID_NDK_HOME/toolchains/arm-linux-androideabi-4.6/prebuilt/linux-x86_6
4/bin</span>

**b. Open Makefile in KERNEL_SOURCE/goldfish/, and find these: **

>ARCH ?= $(SUBARCH)
>
>CROSS_COMPILE ?=

Change it to:
 
> ARCH ?= arm
>
> CROSS_COMPILE ?= arm-linux-androideabi-

Save it and exit.
 
**c. Execute the following command in terminal to set compiling configuration:**

>make goldfish_armv7_defconfig

**d. Modify compiling configuration:**

>sudo apt-get install ncurses-dev
>
>make menuconfig

Then you can see a GUI config. 

![Capture1](https://github.com/juzuz/OOM_Killer/blob/master/assets/Capture.PNG)

Open the <em>Compile the kernel with debug info</em> in <em>Kernel hacking</em>

![Capture2](https://github.com/juzuz/OOM_Killer/blob/master/assets/Capture2.PNG)

<em>Enable loadable module support</em> with <em>Forced module loading, Module unloading
and Forced module unloading</em> in it. 

![Capture3](https://github.com/juzuz/OOM_Killer/blob/master/assets/Capture3.PNG)

Save it and exit.

**e. Compile**

>make -j4

**2. Creating System Call to set limit for users.**

To assign the memory limit to a user, I will be using a system call function that takes in two parameters. 

>asmlinkage long sys_MyLimit(uid_t uid, unsigned long mm_max)

uid: is the user id

mm_max for the maximum memory usage for the specified user.

This system call is written within the sys_arm.c file and needs to be declared in the syscalls.h, calls.S and unistd.h files. I have assigned this system call with the number 59.


**3. The modified OOM Killer.**

The crux of this project was understanding the allocation of memory papges on the Linux system and procedures the OOM Killer has to go through before executing. This can be obeserved in the oom_kill.c and page_alloc.c files. 

🌑 The orginal procedure for is as follows:

> __alloc_pages() // Called anytime memory is allocated
>   |--> __alloc_pages_nodemask()
>       |--> __alloc_pages_may_oom()
>           |--> out_of_memory() // Triggers the oom killer if OOM

If the OOM killer is triggered the following heuristic is applied to find the "worst" process to kill:

The key points of the heuristic is:

- The process and its child process are using up a majority of the memory
- Root, kernal and important process have a bias(3% down) to avoid lower the chances of being killed
- Tasks marked as unkillable are avoided 
- Wait if there is a task in the process of being terminated.
*Root and kernal processes usually don't take up more than 3% memory*

The OOM score is determined by these factors, and the simple mathematical equation is:

> OOM_Badness = 10 * % memory used
>
> *The highest score is maxed at 1000*

![OOM_Badness](https://github.com/juzuz/OOM_Killer/blob/master/assets/OOM_Score.PNG)

🌕 The modified version follows similar steps
Within the alloc_pages_nodemask step of the original process, I have added code to monitor the memory usage of each user. Therefore anytime memory is allocated, we will check whether a user has exceeded its memory limit. 

The memory limits are stored within the kernal and is shared with the function.
We recieve information on the uid and its limit and it triggers the new oom function when the memory exceeds the limit.

MODIFIED_OOM FUNCTION

The mod_oom function is written in the oom_kill.c file.
This project did not use the oom_badness score to choose the "worst" process. Instead, it selects the task to kill by chooising the largest task that is not a root or kernel task.

While the oom function is in the process of terminating a task, the mod_oom is called multiple times within the timespan from when the first oom_kill is called and until it actually terminates a task. The task is killed by modifying the task's flag to <em>waiting to be killed</em>.

The kill is not completed immedietly, but has a duration before death. Therefore, we must check that the "worst" process we chose is not already designated to be killed, and if it is, then just skip the kill function and wait.











