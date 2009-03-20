/*
 * Priority Scheduler created by Michael Siegirst
 * Last updated: 3/19/2009.
 */


#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

/*
 * Macros for the priority round-robin scheduler.
 */
#define PRIORITY_SCHEDULER		// Added my Michael Siegrist.

// Priority scheduler macros.
#ifdef PRIORITY_SCHEDULER

#define NUM_PRIORITIES		4	// Maximum number of priorities.
								// Thread priorities range from 1 - (NUM_PRIORITIES-1).

#define PRIORITY_LOW		0
#define PRIORITY_NORMAL		1
#define PRIORITY_HIGH		2
#define PRIORITY_SUPER		3

#define PRIORITY_DEFAULT    PRIORITY_NORMAL
#define PRIORITY_START		((1 << NUM_PRIORITIES) - 1)

#endif

/*
 * Scheduler-related function calls.
 *
 *     scheduler     - run the scheduler and choose the next thread to run.
 *     make_runnable - add the specified thread to the run queue. If it's
 *                     already on the run queue or sleeping, weird things
 *                     may happen. Returns an error code.
 *
 *     print_run_queue - dump the run queue to the console for debugging.
 *
 *     scheduler_bootstrap - initialize scheduler data 
 *                           (must happen early in boot)
 *     scheduler_shutdown -  clean up scheduler data
 *     scheduler_preallocate - ensure space for at least NUMTHREADS threads.
 *                           Returns an error code.
 */

struct thread;

struct thread *scheduler(void);
int make_runnable(struct thread *t);

void print_run_queue(void);

void scheduler_bootstrap(void);
int scheduler_preallocate(int numthreads);
void scheduler_killall(void);
void scheduler_shutdown(void);

#endif /* _SCHEDULER_H_ */
