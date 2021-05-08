# Linux Sempahore 
This is an implementation of a semaphore (synchronization mechanism) for the Linux kernel version 2.6 on i386. The semaphore can be created, used and closed by making syscalls to the kernel.

## Implementing Syscalls in the Linux Kernel

The syscall identifiers are defined in the unistd.h file located at`/include/asm-i386/unistd.h`

Within the file are the added syscall IDs:

```C
// ... 
#define __NR_cs1550_create  325
#define __NR_cs1550_open    326
#define __NR_cs1550_down    327
#define __NR_cs1550_up      328
#define __NR_cs1550_close   329
```

The new syscalls also need defined in `syscall_table.S` located at `/arch/i386/kernel/syscall_table.S`:

```
ENTRY(sys_call_table)

/* ... */

	.long sys_cs1550_create
	.long sys_cs1550_open
	.long sys_cs1550_down
	.long sys_cs1550_up
	.long sys_cs1550_close
```

The actual implementation of the syscalls are defined in the `cs1550.c` and `cs1550.h`. These files are located in the following locations respectively:

* `/kernel/cs1550.c` and
* `/include/linux/cs1550.h`

## Using the Semaphores 

It's a litter easier to consider the s