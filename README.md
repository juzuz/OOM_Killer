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
