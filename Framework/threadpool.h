#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include "interface.h"

typedef struct{
	sem_t *sema;			/* micro scheduler handle */
	void (*cmd) ( c_Thread_t *);		/* the function to be executed */
	c_Thread_t *arg;			/* the arguments of the function */
} MRtask;

/* each entry in the queue contains a list of tasks */
typedef struct MRQUEUE {
	MRtask *task;
	struct MRQUEUE *next;
} MRqueue;

static MRqueue *mrqueue;
static pthread_mutex_t mrqlock = PTHREAD_MUTEX_INITIALIZER;
static sem_t mrqsema;	

void* MRworker(void *);
void MRschedule(int taskcnt, c_Thread_t *arg, void (*cmd) ( c_Thread_t *p));	
void MRqueueCreate(int nr_threads);
void MRenqueue(MRtask *task);
MRtask* MRdequeue(void);
void* MRworker(void *arg);

#endif /*_THREADPOOL_H_*/
