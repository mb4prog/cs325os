/*
 * Scheduler.
 *
 * The default scheduler is very simple, just a round-robin run queue.
 * You'll want to improve it.
 */

#include <types.h>
#include <lib.h>
#include <scheduler.h>
#include <thread.h>
#include <machine/spl.h>
#include <queue.h>


/////////////////////////////////////////////////////
// DEFAULT BEHAVIOR (ROUND ROBIN)

#ifndef PRIORITY_SCHEDULER

/*
 *  Scheduler data
 */

// Queue of runnable threads
static struct queue *runqueue;

/*
 * Setup function
 */
void
scheduler_bootstrap(void)
{
	runqueue = q_create(32);
	if (runqueue == NULL) {
		panic("scheduler: Could not create run queue\n");
	}
}

/*
 * Ensure space for handling at least NTHREADS threads.
 * This is done only to ensure that make_runnable() does not fail -
 * if you change the scheduler to not require space outside the 
 * thread structure, for instance, this function can reasonably
 * do nothing.
 */
int
scheduler_preallocate(int nthreads)
{
	assert(curspl>0);
	return q_preallocate(runqueue, nthreads);
}

/*
 * This is called during panic shutdown to dispose of threads other
 * than the one invoking panic. We drop them on the floor instead of
 * cleaning them up properly; since we're about to go down it doesn't
 * really matter, and freeing everything might cause further panics.
 */
void
scheduler_killall(void)
{
	assert(curspl>0);
	while (!q_empty(runqueue)) {
		struct thread *t = q_remhead(runqueue);
		kprintf("scheduler: Dropping thread %s.\n", t->t_name);
	}
}

/*
 * Cleanup function.
 *
 * The queue objects to being destroyed if it's got stuff in it.
 * Use scheduler_killall to make sure this is the case. During
 * ordinary shutdown, normally it should be.
 */
void
scheduler_shutdown(void)
{
	scheduler_killall();

	assert(curspl>0);
	q_destroy(runqueue);
	runqueue = NULL;
}

/*
 * Actual scheduler. Returns the next thread to run.  Calls cpu_idle()
 * if there's nothing ready. (Note: cpu_idle must be called in a loop
 * until something's ready - it doesn't know whether the things that
 * wake it up are going to make a thread runnable or not.) 
 */
struct thread *
scheduler(void)
{
	// meant to be called with interrupts off
	assert(curspl>0);
	
	while (q_empty(runqueue)) {
		cpu_idle();
	}

	// You can actually uncomment this to see what the scheduler's
	// doing - even this deep inside thread code, the console
	// still works. However, the amount of text printed is
	// prohibitive.
	// 
	//print_run_queue();
	
	return q_remhead(runqueue);
}

/* 
 * Make a thread runnable.
 * With the base scheduler, just add it to the end of the run queue.
 */
int
make_runnable(struct thread *t)
{
	// meant to be called with interrupts off
	assert(curspl>0);

	return q_addtail(runqueue, t);
}

/*
 * Debugging function to dump the run queue.
 */
void
print_run_queue(void)
{
	/* Turn interrupts off so the whole list prints atomically. */
	int spl = splhigh();

	int i,k=0;
	i = q_getstart(runqueue);
	
	while (i!=q_getend(runqueue)) {
		struct thread *t = q_getguy(runqueue, i);
		kprintf("  %2d: %s %p\n", k, t->t_name, t->t_sleepaddr);
		i=(i+1)%q_getsize(runqueue);
		k++;
	}
	
	splx(spl);
}



/////////////////////////////////////////////////////
// PRIORITY SCHEDULER

#else


static struct queue* runqueue[NUM_PRIORITIES];		// Queue of runnable threads.
//static int queuenum;								// Indicator of which priority queue to use.

void
scheduler_bootstrap(void)
{
	int i;

	// Make sure that we have a positive number of priorities.
	assert(NUM_PRIORITIES > 0);

	//// Initialize queuenum to allow for each additional level
	////   to have twice the allowed time as the level below it
	////   (more indices of queuenum allotted to them).
	//queuenum = PRIORITY_START;

	// Initialize all queues.
	for(i = 0; i < NUM_PRIORITIES; i++)
	{
		runqueue[i] = q_create(32);
		if (runqueue[i] == NULL) {
			panic("scheduler: Could not create run queue for priority %d\n", i);
			break;
		}
	}
}

int
scheduler_preallocate(int nthreads)
{
	int ret = 0;	// Value returned.
	int i;

	assert(curspl>0);
	for(i = 0; i < NUM_PRIORITIES; i++)
	{
		ret = q_preallocate(runqueue[i], nthreads);
		if(ret)
			break;
	}

	return(ret);
}

void
scheduler_killall(void)
{
	int i;

	assert(curspl>0);
	for(i = 0; i < NUM_PRIORITIES; i++)
	{
		while (!q_empty(runqueue[i])) {
			struct thread *t = q_remhead(runqueue[i]);
			kprintf("scheduler: Dropping thread %s.\n", t->t_name);
		}
	}
}

void
scheduler_shutdown(void)
{
	int i;

	scheduler_killall();

	assert(curspl>0);
	for(i = 0; i < NUM_PRIORITIES; i++)
	{
		q_destroy(runqueue[i]);
		runqueue[i] = NULL;
	}
}

/*
 * Schedules the threads according to the highest priority threads.
 */
struct thread *
scheduler(void)
{
	int do_idle;
	int i;
	//int k;
	struct thread* t;

	// meant to be called with interrupts off
	assert(curspl>0);
	
	// Check if all queues are empty.
	do
	{
		do_idle = 1;
		for(i = 0; i < NUM_PRIORITIES; i++)
		{
			do_idle &= q_empty(runqueue[i]);
		}

		// Idle if there are no threads to schedule.
		if(do_idle)
			cpu_idle();
	}
	while(do_idle);

	// Uncomment to print queue contents.
	//print_run_queue();
	
	//// Select the queue to get the thread from.
	//k = 1 << NUM_PRIORITIES;
	//i = NUM_PRIORITIES;

	//// Avoid all empty queues.
	//while(q_empty(runqueue[i - 1]))
	//{
	//	k = k >> 1;
	//	i--;
	//}
	//
	//// Select the queue indicated by queuenum.
	//// NOTE: There is an important difference between
	////       calculating k this way and using PRIORITY_START at initialization!
	//while(k - 1 >= queuenum)
	//{
	//	k = k >> 1;
	//	i--;
	//}

	//// Reset queuenum if needed.
	//queuenum--;
	//if(queuenum < 1)
	//	queuenum = PRIORITY_START;


	// Use a multilevel-feedback queue approach to get the first non-empty queue.
	for(i = NUM_PRIORITIES - 1; i > 0; i--)
	{
		if(!q_empty(runqueue[i]))
			break;
	}

	// Return the first thread from the appropriate queue.
	t = q_remhead(runqueue[i]);

	// Modify the priority of the thread to place it in
	//    next-highest queue during the next pass.
	//    Lowest queue acts as round robin.
	if(t->priority > 0)
		t->priority--;

	return(t);
}

int
make_runnable(struct thread *t)
{
	// meant to be called with interrupts off
	assert(curspl>0);

	// Use the priority of t to determine which queue to add it to.
	return(q_addtail(runqueue[t->priority], t));
}

void
print_run_queue(void)
{
	/* Turn interrupts off so the whole list prints atomically. */
	int spl = splhigh();

	int i, j, k;
	for(i = 0; i < NUM_PRIORITIES; i++)
	{
		j = q_getstart(runqueue[i]);
		k = 0;
		
		kprintf("Priority %d:\n", i);
		while (j != q_getend(runqueue[i])) {
			struct thread *t = q_getguy(runqueue[i], j);
			kprintf("  %2d: %s %p\n", k, t->t_name, t->t_sleepaddr);
			j = (j + 1) % q_getsize(runqueue[i]);
			k++;
		}
	}
	
	splx(spl);
}


#endif
