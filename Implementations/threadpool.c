#include "../Framework/threadpool.h"

#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>

/* There is just a single queue for the workers */
void
MRqueueCreate(int nr_threads)
{
	int i;
	pthread_t tid;

	pthread_mutex_lock(&mrqlock);
	sem_init(&mrqsema, 0, 0);
	if ( mrqueue ) {
		pthread_mutex_unlock(&mrqlock);
		fprintf(stderr,"One map-reduce queue allowed\n");
		return;
	}
	mrqueue = NULL;
	/* create a worker thread for each core as specified as system parameter (nr_threads) */
	for (i = 0; i < nr_threads; i++) {
		pthread_create(&tid, NULL, MRworker, NULL);
//		cpu_set_t pset;
//		CPU_ZERO(&pset);
//		CPU_SET(i, &pset);
//		int r = pthread_setaffinity_np(tid, sizeof(cpu_set_t), &pset);
//		assert(r == 0);
	}
	pthread_mutex_unlock(&mrqlock);
}

void
MRenqueue(MRtask *task)
{
	MRqueue *newtask = NULL;
	pthread_mutex_lock(&mrqlock);
	newtask = (MRqueue *) malloc(sizeof(MRqueue));
	if ( newtask == 0) {
		pthread_mutex_unlock(&mrqlock);
		fprintf(stderr, "Could not enlarge the map-reduce queue");
		return;
	}
	newtask->task = task;
	newtask->next = mrqueue;
	mrqueue = newtask;

	pthread_mutex_unlock(&mrqlock);
	/* a task is added for consumption */
	sem_post(&mrqsema);
}

MRtask *
MRdequeue(void)
{
	MRtask *r = NULL;
	MRqueue *temp;

	sem_wait(&mrqsema);
	pthread_mutex_lock(&mrqlock);

	//assert(mrqueue);
	temp = mrqueue;
	r = mrqueue->task;
	mrqueue = mrqueue->next;
	free(temp);

	pthread_mutex_unlock(&mrqlock);

	//assert(r);
	return r;
}

void*
MRworker(void *arg)
{
	MRtask *task;
	(void) arg;
	while(1) {
		task = MRdequeue();
		(task->cmd) (task->arg);
		sem_post(task->sema);
	}
	return NULL;
}

/* schedule the tasks and return when all are done */
void
MRschedule(int taskcnt, c_Thread_t *arg, void (*cmd) ( c_Thread_t *p))
{
	int i;
	sem_t sema;
	MRtask *task = malloc(taskcnt * sizeof(MRtask));

	if (mrqueue == 0)
		MRqueueCreate(taskcnt);

	sem_init(&sema, 0, 0);
	for (i = 0; i < taskcnt; i++) {
		task[i].sema = &sema;
		task[i].cmd = cmd;
		task[i].arg = arg ? &arg[i] : NULL;
		MRenqueue(&task[i]);
	}
	/* waiting for all report result */
	for (i = 0; i < taskcnt; i++)
		sem_wait(&sema);
	sem_destroy(&sema);
	free(task);
}

