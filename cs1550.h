// Colby King
// CS1550 Project 1 - Semaphores
// Dr. Mosse
#include <linux/smp_lock.h>

struct cs1550_sem
{
   int value;
   long sem_id;
   spinlock_t lock;
   char key[32];
   char name[32];
   struct proc_node* head;
   struct proc_node* tail;
};

// node for doubly linked list of global semaphores 
struct sem_node
{
	struct cs1550_sem* sem;
	struct sem_node* prev;
	struct sem_node* next;
};


// node for linked list of processes. 
// one of these will belong to each created semaphore.
struct proc_node
{
	struct task_struct* proc;
	struct proc_node* next;
};


