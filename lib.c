#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "threadlib.h"
#include "threadimp.h"
#include <sys/time.h>


#ifndef __x86_64__
#error "NOT 64bit" 
#endif


//#define DEBUG
#undef DEBUG /* uncomment when you are done! */

/* 
 * Idea here is that, if DEBUG is defined PRINT will be 
 * printf else it would be nothing. 
 * Use PRINT to print all your debug information and when you 
 * undefine DEBUG all that code will be gone :) 
 */ 
#ifdef DEBUG
 #define PRINT   printf
#else 
 #define PRINT(...)
#endif



tcb_t head=NULL;
tcb_t first_thread=NULL;
int thread_count=0;
long long time = 0;
int etime = 0;


long long current_timestamp(void) {
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;
    return milliseconds;
}

/* Track the current thread
 */ 
tcb_t __current_thread;

/* You might need other information about the 
 * thread system as well. 
 */
tcb_t current_thread(void)
{
  return __current_thread;
}

void set_current(tcb_t current)
{
  __current_thread = current;
}

/*Creates a new tcb*/
tcb_t newtcb(void)
{ 
  tcb_t dummy = (tcb_t) malloc(sizeof(struct tcb));
	return dummy;
}

int append(tcb_t newguy)
{
  if(thread_count == 1){
      head= newguy;
      first_thread= newguy;
      newguy->next= first_thread;
      newguy->previous = first_thread;
      return 0;
   }else if(thread_count == 2){
      head->next = newguy;
      head->previous = newguy;
      newguy->previous = head;
      newguy->next = head;
      head=newguy;
      return 0;
   }else{
      head->next = newguy;
      newguy->previous = head;
      head = newguy;
      head->next = first_thread;
      first_thread->previous =newguy;
      return 0;
   }
  return -1; 
}

int delete_item (tcb_t thread)
{
  thread->previous->next = thread->next;
  thread->next->previous = thread->previous;
  head=thread->next;
  free(thread);
  return -1;
}


tcb_t schedule(tcb_t current)
{
  /* decide who should run next */
  tcb_t temp = current;
  tcb_t retu = current;
  etime = current->execution_time;
  while(current->next != temp){
    current = current->next;
    if(current->execution_time < etime ){
      retu = current;
    }
  }
  
  return retu;
  //return current->next; 

}

/**
 * assembly code for switching 
 * @sp -- new stack to switch 
 * return sp of the old thread
 * 
 * Switching 
*/

void machine_switch(tcb_t new_thread /* addr. of new TCB */, 
		    tcb_t current_thread /* addr. of old TCB */);

void switch_threads(tcb_t new_thread /* addr. of new TCB */, 
		    tcb_t old_thread /* addr. of old TCB */);
		    
void switch_to(tcb_t new_thread);


void switch_threads(tcb_t new_thread /* addr. of new TCB */, tcb_t current_thread /* addr. of old TCB */) {

   /* This is basically a front end to the low-level assembly code to switch. */
   machine_switch(new_thread,current_thread);

  // assert(!printf("Implement %s",__func__));

}


/*********************************************************
 *                 Thread creation etc 
 *********************************************************/

/* Notes: make sure to have sufficient space for the stack
 * also it needs to be aligned 
 */

#include <stdlib.h>
#include <assert.h>

/*
 * allocate some space for thread stack.
 * malloc does not give size aligned memory 
 * this is some hack to fix that.
 * You can use the code as is. 
 */

void * malloc_stack(void) 
{
  /* allocate something aligned at 16
   */
   void *ptr = malloc(STACK_SIZE + 16);
   if (!ptr) return NULL;
  return ptr;
}


int create_thread(void (*ip)(void)) {
	
	long int * Stack; 
	Stack = malloc_stack();
	if(!Stack) return -1;

   //tcb_t new_thread= (tcb_t) malloc(sizeof(tcb_));
   tcb_t new_thread = newtcb();
   if(!new_thread) return -1;
   
   new_thread->bp=Stack;
   Stack=(void *) (((long int)Stack & (-1 << 4) ) + STACK_SIZE -1);
   *(Stack)= (long int) ip;
   Stack--;
   *(Stack)= (long int )Stack +16;
   new_thread->sp = Stack - 14;

   thread_count++;

   if(append(new_thread)== 0){
     set_current(head);
   }
   new_thread->timestamp = current_timestamp();
   new_thread->execution_time = 0;
   
  /**
   * Stack layout: last slot should contain the return address and I should have some space 
   * for callee saved registers. Also, note that stack grows downwards. So need to start from the top. 
   * Should be able to use this code without modification Basic idea: C calling convention tells us the top 
   * most element in the stack should be return ip. So we create a stack with the address of the function 
   * we want to run at this slot. 
   */

	return 0;
}

void yield(){
  /* thread wants to give up the CPUjust call the scheduler to pick the next thread. */
  tcb_t temp = head;
  head= schedule(head); /* obtaining the tcb of next thread */
  set_current(head);
  time = current_timestamp();
  temp->execution_time = time - temp->timestamp;
  head->timestamp = time;
  switch_threads(head, temp);
  #ifdef DEBUG
    //printf("%p  and %p\n", head, head->previous );
    //printf("%lld  and %d\n", time , head->execution_time);
  #endif
  
}


void delete_thread(void){

   free(head->bp);
   if(thread_count==1){  
    free(head);
    thread_count=0;
    exit(0);
   }
   else{
      #ifdef DEBUG
        printf("%p  and %p\n",head,head->previous );
      #endif
      delete_item(head); /*switch head to new one and free old one*/
      thread_count--;
      switch_to(head);
     

   }

  /* When a user-level thread calls this you should not 
   * let it run any more but let others run
   * make sure to exit when all user-level threads are dead */ 

 // assert(!printf("Implement %s",__func__));
 
}


void stop_main(void)
{ 
  #ifdef DEBUG
    printf("%p  and %p and %p\n", head, head->next, first_thread );
    
  #endif
	switch_to(head);


}
