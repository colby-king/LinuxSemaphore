# Linux Sempahore Implementation
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

Wrapping the syscalls with functions makes them easier to read when working in userspace. You can do do something like this:

```C
long create(int value, char name[32], char key[32]) {
  return syscall(__NR_cs1550_create, value, name, key);
}

long open(char name[32], char key[32]) {
  return syscall(__NR_cs1550_open, name, key);
}

long down(long sem_id) {
  return syscall(__NR_cs1550_down, sem_id);
}

long up(long sem_id) {
  return syscall(__NR_cs1550_up, sem_id);
}

long close(long sem_id) {
  return syscall(__NR_cs1550_close, sem_id);
}
```

### Initializing a Semaphore

Using the code above you can initialize a semaphore by calling the create function with the following parameters
* **Value**: The count value for the semaphore 
* **Name**: Name for it. You'll use this to retrieve it
* **Key**: A secret key to protect access 

```C
char key[32] = "secret ley";
long lockId = create(1, "BinarySemaphore", key);
```

### Accessing a Protected Resource

Once the semaphore is initialized, you can use it to protect resources or shared variables in your code. For example, if we want to create a critical section with the binary sempahore defined about, we can do this:

```C
down(lockId)
// critical section
// Do stuff that requires synchronized access
up(lockId)
```

The **down** function will decrement the semaphore's value and will queue the thread if it's value is less than one (another thread is executing in the critical section)

The **up** function will increment the value by one and dequeue any blocked processes waiting on our lock. 

### Closing the Semaphore 

To deallocate the Semaphore from a system-wide list in the kernel, you call the **close** function. To do so, you just need to specify the semaphore's name given when it was created. 

```C
close(lockId)
```











