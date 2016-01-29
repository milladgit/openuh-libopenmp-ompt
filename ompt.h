/*
 OpenMP OMPT Support for Open64's OpenMP runtime library

 Copyright (C) 2014 University of Houston.

 This program is free software; you can redistribute it and/or modify it
 under the terms of version 2 of the GNU General Public License as
 published by the Free Software Foundation.

 This program is distributed in the hope that it would be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 Further, this software is distributed without any warranty that it is
 free of the rightful claim of any third person regarding infringement
 or the like.  Any license provided herein, whether implied or
 otherwise, applies only to this software file.  Patent licenses, if
 any, provided herein do not apply to combinations of this program with
 other software, or any other product whatsoever.

 You should have received a copy of the GNU General Public License along
 with this program; if not, write the Free Software Foundation, Inc., 59
 Temple Place - Suite 330, Boston MA 02111-1307, USA.

 Contact information:
 http://www.cs.uh.edu/~hpctools
*/


#ifndef __OMPT_H_
#define __OMPT_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


//typedef unsigned int uint64_t;
#define OMPT_API


/*
 *
 * Definitions
 *
 */

typedef uint64_t ompt_thread_id_t;
typedef uint64_t ompt_wait_id_t;
typedef uint64_t ompt_parallel_id_t;
typedef uint64_t ompt_task_id_t;

typedef struct ompt_frame_s {
	void *exit_runtime_frame; /* next frame is user code */
	void *reenter_runtime_frame; /* previous frame is user code */
} ompt_frame_t;



/*
 *
 * States
 *
 */

typedef enum {
	/* work state (0..15) */
	ompt_state_work_serial = 0x00, 				/* working outside parallel */
	ompt_state_work_parallel = 0x01, 			/* working within parallel */
	ompt_state_work_reduction = 0x02, 			/* performing a reduction */

	/* idle (16..31) */
	ompt_state_idle = 0x10, 					/* waiting for work */

	/* overhead states (32..63) */
	ompt_state_overhead = 0x20, 				/* non-wait overhead */

	/* barrier wait states (64..79) */
	ompt_state_wait_barrier = 0x40, 			/* generic barrier */
	ompt_state_wait_barrier_implicit = 0x41, 	/* implicit barrier */
	ompt_state_wait_barrier_explicit = 0x42, 	/* explicit barrier */

	/* task wait states (80..95) */
	ompt_state_wait_taskwait = 0x50, 			/* waiting at a taskwait */
	ompt_state_wait_taskgroup = 0x51, 			/* waiting at a taskgroup */

	/* mutex wait states (96..111) */
	ompt_state_wait_lock = 0x60, 				/* waiting for lock */
	ompt_state_wait_nest_lock = 0x61, 			/* waiting for nest lock */
	ompt_state_wait_critical = 0x62, 			/* waiting for critical */
	ompt_state_wait_atomic = 0x63, 				/* waiting for atomic */
	ompt_state_wait_ordered = 0x64, 			/* waiting for ordered */

	/* misc (112.127) */
	ompt_state_undefined = 0x70, 				/* undefined thread state */
	ompt_state_first = 0x71, 					/* initial enumeration state */


} ompt_state_t;



/*
 *
 * Events
 *
 */

typedef enum {
	/*--- Mandatory Events ---*/
	ompt_event_parallel_begin = 1, 				/* parallel create */
	ompt_event_parallel_end = 2, 				/* parallel exit */

	ompt_event_task_begin = 3, 					/* task create */
	ompt_event_task_end = 4, 					/* task destroy */

	ompt_event_thread_begin = 5, 				/* thread begin */
	ompt_event_thread_end = 6, 					/* thread end */


	ompt_event_control = 7, 					/* support control calls */
	ompt_event_runtime_shutdown = 8, 			/* runtime shutdown */

	/*--- Optional Events (blame shifting) ---*/
	ompt_event_idle_begin = 9, 					/* begin idle state */
	ompt_event_idle_end = 10, 					/* end idle state */

	ompt_event_wait_barrier_begin = 11,			/* begin wait at barrier */
	ompt_event_wait_barrier_end = 12,			/* end wait at barrier */
	ompt_event_wait_taskwait_begin = 13, 		/* begin wait at taskwait */
	ompt_event_wait_taskwait_end = 14, 			/* end wait at taskwait */
	ompt_event_wait_taskgroup_begin = 15,		/* begin wait at taskgroup */
	ompt_event_wait_taskgroup_end = 16,			/* end wait at taskgroup */

	ompt_event_release_lock	= 17, 				/* lock release */
	ompt_event_release_nest_lock_last = 18, 	/* last nest lock release */
	ompt_event_release_critical = 19, 			/* critical release */
	ompt_event_release_atomic = 20, 			/* atomic release */
	ompt_event_release_ordered = 21, 			/* ordered release */

	/*--- Optional Events (synchronous events) --- */
	ompt_event_implicit_task_begin = 22, 		/* implicit task create */
	ompt_event_implicit_task_end = 23, 			/* implicit task destroy */

	ompt_event_initial_task_begin = 24, 		/* initial task create */
	ompt_event_initial_task_end = 25, 			/* initial task destroy */

	ompt_event_task_switch = 26, 				/* task switch */

	ompt_event_loop_begin = 27, 				/* task at loop begin */
	ompt_event_loop_end = 28, 					/* task at loop end */

	ompt_event_sections_begin = 29, 			/* task at section begin */
	ompt_event_sections_end = 30, 				/* task at section end */

	ompt_event_single_in_block_begin = 31, 		/* task at single begin */
	ompt_event_single_in_block_end = 32, 		/* task at single end */
	ompt_event_single_others_begin = 33, 		/* task at single begin */
	ompt_event_single_others_end = 34, 			/* task at single end */

	ompt_event_workshare_begin = 35, 			/* task at workshare begin */
	ompt_event_workshare_end = 36, 				/* task at workshare end */

	ompt_event_master_begin = 37, 				/* task at master begin */
	ompt_event_master_end = 38, 				/* task at master end */

	ompt_event_barrier_begin = 39, 				/* task at barrier begin */
	ompt_event_barrier_end = 40, 				/* task at barrier end */

	ompt_event_taskwait_begin = 41, 			/* task at taskwait begin */
	ompt_event_taskwait_end = 42, 				/* task at task wait end */

	ompt_event_taskgroup_begin = 43, 			/* task at taskgroup begin */
	ompt_event_taskgroup_end = 44, 				/* task at taskgroup end */

	ompt_event_release_nest_lock_prev = 45, 	/* prev nest lock release */


	ompt_event_wait_lock = 46, 					/* lock wait */
	ompt_event_wait_nest_lock = 47, 			/* nest lock wait */
	ompt_event_wait_critical = 48, 				/* critical wait */
	ompt_event_wait_atomic = 49, 				/* atomic wait */
	ompt_event_wait_ordered	 = 50, 				/* ordered wait */

	ompt_event_acquired_lock = 51, 				/* lock acquired */
	ompt_event_acquired_nest_lock_first = 52, 	/* 1st nest lock acquired */
	ompt_event_acquired_nest_lock_next = 53, 	/* next nest lock acquired */
	ompt_event_acquired_critical = 54, 			/* critical acquired */
	ompt_event_acquired_atomic = 55, 			/* atomic acquired */
	ompt_event_acquired_ordered = 56, 			/* ordered acquired */

	ompt_event_init_lock = 57, 					/* lock init */
	ompt_event_init_nest_lock = 58, 			/* nest lock init */

	ompt_event_destroy_lock = 59, 				/* lock destruction */
	ompt_event_destroy_nest_lock = 60, 			/* nest lock destruction */

	ompt_event_flush = 61 						/* after executing flush */

} ompt_event_t;



/*
 *
 * Interfaces
 *
 */

typedef void (*ompt_callback_t)(void);


/* initialization */
typedef void (*ompt_interface_fn_t)(void);

typedef ompt_interface_fn_t (*ompt_function_lookup_t)(const char *entry_point /* entry point to look up */
);


/* threads */
typedef void (*ompt_thread_callback_t)( /* for thread */
ompt_thread_id_t thread_id /* ID of thread */
);

typedef enum ompt_thread_type_e {
	ompt_thread_initial = 1,
	ompt_thread_worker = 2,
	ompt_thread_other = 3
} ompt_thread_type_t;

typedef void (*ompt_thread_type_callback_t)( /* for thread */
ompt_thread_type_t thread_type, /* type of thread */
ompt_thread_id_t thread_id /* ID of thread */
);

typedef void (*ompt_wait_callback_t)( /* for wait */
ompt_wait_id_t wait_id /* wait ID */
);


/* parallel & workshares */
typedef void (*ompt_parallel_callback_t)( /* for inside parallel */
ompt_parallel_id_t parallel_id, /* ID of parallel region */
ompt_task_id_t task_id /* ID of task */
);

typedef void (*ompt_new_workshare_callback_t) ( /* for workshares */
ompt_parallel_id_t parallel_id, /* ID of parallel region */
ompt_task_id_t task_id, /* ID of task */
void *workshare_function /* pointer to outlined function */
);

typedef void (*ompt_new_parallel_callback_t) ( /* for new parallel */
ompt_task_id_t parent_task_id, /* ID of parent task */
ompt_frame_t *parent_task_frame, /* frame data of parent task */
ompt_parallel_id_t parallel_id, /* ID of parallel region */
uint32_t requested_team_size, /* requested number of threads */
void *parallel_function /* pointer to outlined function*/
);


/* tasks */
typedef void (*ompt_task_callback_t) ( /* for tasks */
ompt_task_id_t task_id /* ID of task */
);

typedef void (*ompt_task_switch_callback_t) ( /* for task switch */
ompt_task_id_t suspended_task_id, /* ID of suspended task */
ompt_task_id_t resumed_task_id /* ID of resumed task */
);

typedef void (*ompt_new_task_callback_t) ( /* for new tasks */
ompt_task_id_t parent_task_id, /* ID of parent task */
ompt_frame_t *parent_task_frame, /* frame data for parent task */
ompt_task_id_t new_task_id, /* ID of created task */
void *new_task_function /* pointer to outlined function */
);


/* program */
typedef void (*ompt_control_callback_t)( /* for control */
uint64_t command, /* command of control call */
uint64_t modifier /* modifier of control call */
);

/*
 *
 * OMPT Inquiry and Control APIs
 *
 */

/* callback management */
typedef OMPT_API int (*ompt_set_callback_t)( /* register a callback for an event */
ompt_event_t event, /* the event of interest */
ompt_callback_t callback /* function pointer for the callback */
);
typedef OMPT_API int (*ompt_get_callback_t)( /* return the current callback for an event (if any) */
ompt_event_t event, /* the event of interest */
ompt_callback_t *callback /* pointer to receive the return value */
);

/* state inquiry */
typedef OMPT_API int (*ompt_enumerate_state_t)( /* extract the set of states supported */
ompt_state_t current_state, /* current state in the enumeration */
ompt_state_t *next_state, /* next state in the enumeration */
const char **next_state_name /* string description of next state */
);

/* thread inquiry */
typedef OMPT_API ompt_thread_id_t (*ompt_get_thread_id_t)( /* identify the current thread */
void);

typedef OMPT_API ompt_state_t (*ompt_get_state_t)( /* get the state for a thread */
ompt_wait_id_t *wait_id /* for wait states: identify what awaited */
);

typedef OMPT_API void * (*ompt_get_idle_frame_t)( /* identify the idle frame (if any) for a thread */
void);

/* parallel region inquiry */
typedef OMPT_API ompt_parallel_id_t (*ompt_get_parallel_id_t)( /* identify a parallel region */
int ancestor_level /* how many levels the ancestor is removed from the current region */
);

typedef OMPT_API int (*ompt_get_parallel_team_size_t)( /* query # threads in a parallel region */
int ancestor_level /* how many levels the ancestor is removed from the current region */
);

/* task inquiry */
typedef OMPT_API ompt_task_id_t * (*ompt_get_task_id_t)( /* identify a task */
int depth /* how many levels removed from the current task */
);


typedef OMPT_API ompt_frame_t * (*ompt_get_task_frame_t)(
	int depth /* how many levels removed from the current task */
);

/*
 *
 * Initialization
 *
 */

int ompt_initialize(
		ompt_function_lookup_t lookup, /* function to look up OMPT API routines by name */
		const char *runtime_version, /* OpenMP runtime version string */
		unsigned int ompt_version /* integer that identifies the OMPT revision */
);

ompt_interface_fn_t ompt_lookup(const char *interface_function_name);


/*
 * Control Function
 */

/* Enumeration for states control function */
enum {
	OMPT_CONTROL_START_RESTART,
	OMPT_CONTROL_PAUSE,
	OMPT_CONTROL_FLUSH_CONTINUE,
	OMPT_CONTROL_PERMANENTLY_OFF
};

void ompt_control(uint64_t command, uint64_t modifier);



#ifdef __cplusplus
}
#endif

#endif
