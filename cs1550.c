// Colby King
// CS1550 Project 1 - Semaphores
// Dr. Mosse
#include <linux/cs1550.h>
#include <linux/string.h>


// Global number to track semaphor IDs 
long global_sem_id = 1;

// Lock to protect global_sem_id and the semlist
DEFINE_SPINLOCK(global_lock);

// Global linked list to track created semaphores 
struct sem_node* semlist_head = NULL;
struct sem_node* semlist_tail = NULL;

// String helper method -- Checks if two strings are equal
// Thanks Jarrett Billingsley!
int streq(const char* a, const char* b) {
	return strcmp(a, b) == 0;
}

/* Looks up a cs1550_sem by in the global semlist (by name) by
 * performing a linear search
 *
 * @param name - name to search by
 * @return cs1550_sem if name matches; otherwise, null
 */
struct sem_node* lookup_sem_node_by_name(const char* name){
	struct sem_node* n;
	// Perform read atomically to avoid stale data
	for(n = semlist_head; n != NULL; n = n->next){
		if(streq(n->sem->name, name)){
			return n;
		}
	}
	return NULL;
}

/* Looks up a cs1550_sem by in the global semlist (by id) by
 * performing a linear search
 *
 * @param name - name to search by
 * @return cs1550_sem if name matches; otherwise, null
 */
struct sem_node* lookup_sem_node_by_id(long sem_id){
	struct sem_node* n;
	for(n = semlist_head; n != NULL; n = n->next){
		if(n->sem->sem_id == sem_id){
			return n;
		}
	}
	return NULL;
}

/* Allocates for and initializes a sem_node. These are the nodes
 * used in the global linked list of system cs1550 semaphores. 
 *
 * @param sem - cs1550_sem pointer to initialize semnode
 * @return node - pointer to new sem_node or null if unsuccessful 
 */
struct sem_node* sem_node_init(struct cs1550_sem* sem){
	struct sem_node* node;	
	//Allocate memory for the sem_node 
	node = (struct sem_node*) kmalloc(sizeof(struct sem_node), GFP_KERNEL);
	if(!node){
		return NULL;
	} 
	//Initialize variables 
	node->sem = sem;
	node->prev = NULL;
	node->next = NULL;
	return node;
}

/* Saves a new cs1550_sem to the global list
 * of system semaphores. 
 *
 * @param sem - cs1550_sem pointer to initialize sem_node
 * @return node - pointer to new node containing the cs1550_sem; 
 * 				  returns NULL if unsuccessful. 
 */
struct sem_node* save_cs1550_sem(struct cs1550_sem* sem){
	struct sem_node* node = sem_node_init(sem);
	if(!node){
		return NULL;
	}
	// Initialize sem_node list if empty
	if(!semlist_head || !semlist_tail){
		semlist_head = node;
		semlist_tail = semlist_head;
	// Otherwise, append sem_node to end of the list
	} else { 
		semlist_tail->next = node;
		node->prev = semlist_tail;
		semlist_tail = semlist_tail->next;
	}
	return node;
}

/* Deletes an existing cs1550_sem from the global list
 * and deallocates. 
 *
 * @param sem - cs1550_sem pointer to node_to_delete
 * @return 0 if successful; -1 otherwise 
 */
void delete_cs1550_sem(struct sem_node* node_to_delete){
	int is_head, is_tail, only_node;
	is_head = (node_to_delete == semlist_head);
	is_tail = (node_to_delete == semlist_tail);
	only_node = (is_head && is_tail);

	if(only_node){
		semlist_head = NULL;
		semlist_tail = NULL;
	} else if(is_head){
		semlist_head = node_to_delete->next;
		semlist_head->prev = NULL;
	} else if(is_tail){
		semlist_tail = semlist_tail->prev;
		semlist_tail->next = NULL;
	} else {
		node_to_delete->prev->next = node_to_delete->next;
		node_to_delete->next->prev = node_to_delete->prev;
	}
	kfree(node_to_delete);
}


/* Allocates for and initializes a new cs1550_sem. 
 *
 * @param value - value to initialize cs1550_sem to
 * @param name - name of the cs1550_sem
 * @param key - security key assoicated with the cs1550_sem
 *
 * @return sem - pointer to new cs1550_sem
 */
struct cs1550_sem* cs1550_sem_init(int value, char name[32], char key[32]){
	struct cs1550_sem* new_sem;

	//Allocate for the semaphore 
	new_sem = (struct cs1550_sem*) kmalloc(sizeof(struct cs1550_sem), GFP_KERNEL);
	if(!new_sem){
		return NULL;
	}
	//Initalize semaphore variables
	new_sem->value = value;
	strncpy(new_sem->key, key, 32);
	strncpy(new_sem->name, name, 32);
	new_sem->head = NULL;
	new_sem->tail = NULL;
	spin_lock_init(&new_sem->lock);
	//Increment global ID atomically
	spin_lock(&global_lock); 
	new_sem->sem_id = global_sem_id;
	global_sem_id++;		 
	spin_unlock(&global_lock); 

	return new_sem;
}

/* Appends a proc_node to the end of a cs1550_sem's
 * process list. 
 *
 * @param sem - sem whose process list is being appended to
 * @param proc - task_struct to append
 *
 * @return ...
 */
long append_to_process_list(struct cs1550_sem* sem, struct task_struct* proc){
	struct proc_node* new_proc_node;
	// Allocate & initialize proc_node
	new_proc_node = (struct proc_node*) kmalloc(sizeof(struct proc_node), GFP_KERNEL);
	if(!new_proc_node){
		return -1;
	}
	new_proc_node->proc = proc;

	// Case where list is not empty
	if(sem->head){
		sem->tail->next = new_proc_node;
		sem->tail = sem->tail->next;
	} else {
		sem->head = new_proc_node;
		sem->tail = new_proc_node;
	}
	return 0;
}


/* Gets the next sleeping process from a cs1550_sem's
 * process queue
 *
 * @param sem - cs1550_sem to get next task struct from
 *
 * @return next task_struct from cs1550_sem queue
 */
struct task_struct* get_next_sleeping_process(struct cs1550_sem* sem){
	struct task_struct* next_proc;
	int only_node;

	// Return if we have nothing waiting in the queue
	if(!sem->head){
		return NULL;
	}

	only_node = (sem->head == sem->tail);
	next_proc = sem->head->proc;
	kfree(sem->head);
	if(only_node){
		sem->head = NULL;
		sem->tail = NULL;
	} else {
		sem->head = sem->head->next;
	}

	return next_proc;
}


/* This syscall creates a new semaphore and stores the provided key to protect access to the semaphore. The integer value is used to initialize the semaphore's value. The function returns the identifier of the created semaphore, which can be used to down and up the semaphore. */
asmlinkage long sys_cs1550_create(int value, char name[32], char key[32]){
	struct cs1550_sem* sem;
	struct sem_node* sem_node;

	// Check validity of args 
	if(value < 0 || streq(name, "") || streq(key, "")){
		return -1;
	}

	sem = cs1550_sem_init(value, name, key);
	// return if kmalloc fails for cs1550_sem
	if(!sem){
		return -1;
	}

	// Save new cs1550_sem to the global list. 
	spin_lock(&global_lock); 
	sem_node = save_cs1550_sem(sem);
	spin_unlock(&global_lock); 

	// Deallocate for cs1550_sem if saving fails.
	// This case occurs if kmalloc fails in sem_node_init.
	if(!sem_node){
		kfree(sem);
		return -1; 
	}

	return sem->sem_id;
}
/* This syscall opens an already created semaphore by providing the semaphore name and the correct key. The function returns the identifier of the opened semaphore if the key matches the stored key or -1 otherwise. */
asmlinkage long sys_cs1550_open(char name[32], char key[32]){
	struct sem_node* sem_node;
	struct cs1550_sem* sem;

	// Search atomically to prevent use-after-free
	spin_lock(&global_lock); 
	sem_node = lookup_sem_node_by_name(name);
	spin_lock(&global_lock);


	// Case where cs1550_sem not found. 
	if(!sem_node){
		return -1;
	}

	sem = sem_node->sem;
	// Check security key
	if(streq(sem->key, key)){
		return sem->sem_id;
	}

	// security key doesn't match
  	return -1; 
}
/* This syscall implements the down operation on an already opened semaphore using the semaphore identifier obtained from a previous call to sys_cs1550_create or sys_cs1550_open. The function returns 0 when successful or -1 otherwise (e.g., if the semaphore id is invalid or if the queue is full). Please check the lecture slides for the pseudo-code of the down operation. */
asmlinkage long sys_cs1550_down(long sem_id){
	struct sem_node* sem_node;
	struct cs1550_sem* sem;
	struct task_struct* curproc;
	long syscall_status = 0;


	spin_lock(&global_lock);
	// Lookup needs to be atomic w/ the semaphore modification
	sem_node = lookup_sem_node_by_id(sem_id);

	// Case where cs1550_sem is not found
	if(!sem_node){
		spin_unlock(&global_lock); 
		return -1;
	}

	sem = sem_node->sem;
	spin_lock(&sem->lock);
	sem->value--;
	if(sem->value < 0){
		curproc = current;
		//queue process and get it ready for sleeping
		syscall_status = append_to_process_list(sem, curproc);

		// Abort operation if kernel fails to allocate for proc_node.
		if(syscall_status < 0){
			sem->value++;
			spin_unlock(&sem->lock);
			spin_unlock(&global_lock);
			return -1;
		}

		// Set this while we have the lock to prevent lost wakeup
		set_current_state(TASK_INTERRUPTIBLE);
		// Unlock here to prevent deadlock!
		spin_unlock(&sem->lock);
		spin_unlock(&global_lock); 
		schedule(); // Sleep
	} else {
		// Dont forget to release lock! 
		spin_unlock(&sem->lock); 
		spin_unlock(&global_lock);
	}

	return syscall_status;
}
/* This syscall implements the up operation on an already opened semaphore using the semaphore identifier obtained from a previous call to sys_cs1550_create or sys_cs1550_open. The function returns 0 when successful or -1 otherwise (e.g., if the semaphore id is invalid). Please check the lecture slides for pseudo-code of the up operation. */
asmlinkage long sys_cs1550_up(long sem_id){
	struct sem_node* sem_node;
	struct cs1550_sem* sem;
	struct task_struct* sleeping_task;


	spin_lock(&global_lock);
	// Lookup needs to be atomic w/ semaphore modification
	sem_node = lookup_sem_node_by_id(sem_id);

	// Case where sem is not found by provided id
	if(!sem_node){
		// Release lock if we fail. 
		spin_unlock(&global_lock); 
		return -1;
		
	}

	sem = sem_node->sem;
	spin_lock(&sem->lock);
	sem->value++;
	if(sem->value <= 0){
		sleeping_task = get_next_sleeping_process(sem);
		wake_up_process(sleeping_task);
	}
	spin_unlock(&sem->lock); 
	spin_unlock(&global_lock); 
  	return 0;
}
/* This syscall removes an already created semaphore from the system-wide semaphore list using the semaphore identifier obtained from a previous call to sys_cs1550_create or sys_cs1550_open. The function returns 0 when successful or -1 otherwise (e.g., if the semaphore id is invalid or the semaphore's process queue is not empty). */
asmlinkage long sys_cs1550_close(long sem_id){
	struct sem_node* sem_node;
	
	spin_lock(&global_lock); 
	sem_node = lookup_sem_node_by_id(sem_id);
	// Return when node is not found or if the process queue is not empty
	if(!sem_node || sem_node->sem->head != NULL){
		// Release lock if we do not find
		spin_unlock(&global_lock);
		return -1;
	} 
	delete_cs1550_sem(sem_node);
	spin_unlock(&global_lock); 

  	return 0;
}
