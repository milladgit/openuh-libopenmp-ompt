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


#include "ompt.h"
#include "omp_rtl.h"


/*
 * Definitions
 */

volatile int __ompt_is_initialized = 0;
volatile int __ompt_track_monitoring = 0;
ompt_callback_t ompt_callback_list[ompt_event_flush + 1];


ompt_task_id_t __ompt_suspended_task_id;
ompt_task_id_t __ompt_resumed_task_id;


extern ompt_parallel_id_t __ompt_parallel_region_id;
extern int __ompt_team_size;
extern omp_micro __ompt_entry_func;



/*
 * Internal API Functions
 */

omp_team_t * __ompt_get_parallel_region_ancestor(int ancestor_level) {
	omp_v_thread_t *p_vthread = __ompc_get_current_v_thread();
	omp_team_t *team = p_vthread->team;
	int i;
	for(i=0;i<ancestor_level;i++) {
		if(team == NULL)
			return NULL;
		team = team->parent;
	}
	return team;
}

omp_task_t *__ompt_get_task_ancestor(int ancestor_level) {
	omp_v_thread_t *p_vthread = __ompc_get_current_v_thread();
	omp_task_t* task = p_vthread->implicit_task;
	int i;
	for(i=0;i<ancestor_level;i++) {
		if(task == NULL)
			return NULL;
		task = task->parent;
	}
	return task;
}


enum {
	NO_TYPE_CALLBACK = 1,
	THREAD_CALLBACK,
	THREAD_TYPE_CALLBACK,
	WAIT_CALLBACK,
	PARALLEL_CALLBACK,
	NEW_WORKSHARE_CALLBACK,
	NEW_PARALLEL_CALLBACK,
	TASK_CALLBACK,
	TASK_SWITCH_CALLBACK,
	NEW_TASK_CALLBACK,
	CONTROL_CALLBACK
};

int __ompt_get_which_event(ompt_event_t event) {
	switch (event) {
	case ompt_event_runtime_shutdown:
		return NO_TYPE_CALLBACK;

	case ompt_event_idle_begin:
	case ompt_event_idle_end:
	case ompt_event_flush:
		return THREAD_CALLBACK;

	case ompt_event_thread_begin:
	case ompt_event_thread_end:
		return THREAD_TYPE_CALLBACK;

	case ompt_event_release_lock:
	case ompt_event_release_nest_lock_last:
	case ompt_event_release_critical:
	case ompt_event_release_ordered:
	case ompt_event_release_atomic:
	case ompt_event_init_lock:
	case ompt_event_init_nest_lock:
	case ompt_event_destroy_lock:
	case ompt_event_destroy_nest_lock:
	case ompt_event_wait_lock:
	case ompt_event_acquired_lock:
	case ompt_event_wait_nest_lock:
	case ompt_event_acquired_nest_lock_first:
	case ompt_event_release_nest_lock_prev:
	case ompt_event_acquired_nest_lock_next:
	case ompt_event_wait_critical:
	case ompt_event_acquired_critical:
	case ompt_event_wait_ordered:
	case ompt_event_acquired_ordered:
	case ompt_event_wait_atomic:
	case ompt_event_acquired_atomic:
		return WAIT_CALLBACK;

	case ompt_event_parallel_begin:
		return NEW_PARALLEL_CALLBACK;

	case ompt_event_parallel_end:
	case ompt_event_wait_barrier_begin:
	case ompt_event_wait_barrier_end:
	case ompt_event_wait_taskwait_begin:
	case ompt_event_wait_taskwait_end:
	case ompt_event_wait_taskgroup_begin:
	case ompt_event_wait_taskgroup_end:
	case ompt_event_implicit_task_begin:
	case ompt_event_implicit_task_end:
	case ompt_event_loop_end:
	case ompt_event_sections_end:
	case ompt_event_single_in_block_end:
	case ompt_event_single_others_begin:
	case ompt_event_single_others_end:
	case ompt_event_workshare_end:
	case ompt_event_master_begin:
	case ompt_event_master_end:
	case ompt_event_barrier_begin:
	case ompt_event_barrier_end:
	case ompt_event_taskwait_begin:
	case ompt_event_taskwait_end:
	case ompt_event_taskgroup_begin:
	case ompt_event_taskgroup_end:
		return PARALLEL_CALLBACK;

	case ompt_event_task_begin:
		return NEW_TASK_CALLBACK;

	case ompt_event_task_end:
	case ompt_event_initial_task_begin:
	case ompt_event_initial_task_end:
		return TASK_CALLBACK;

	case ompt_event_task_switch:
		return TASK_SWITCH_CALLBACK;

	case ompt_event_loop_begin:
	case ompt_event_sections_begin:
	case ompt_event_single_in_block_begin:
	case ompt_event_workshare_begin:
		return NEW_WORKSHARE_CALLBACK;

	case ompt_event_control:
		return CONTROL_CALLBACK;
	};

	return 0;
}

void __ompt_event_callback(ompt_event_t event) {
	if(__ompt_track_monitoring == 0)
		return;

	if(event > ompt_event_flush || ompt_callback_list[event] == NULL)
		return;

	int which_event = __ompt_get_which_event(event);

	if(which_event == NO_TYPE_CALLBACK) {

		(*ompt_callback_list[event])();

	} else if(which_event == THREAD_CALLBACK) {

		ompt_thread_callback_t new_func = (ompt_thread_callback_t)ompt_callback_list[event];
		ompt_thread_id_t thread_id = (ompt_thread_id_t)__omp_myid + 1;
		(*new_func)(thread_id);

	} else if(which_event == THREAD_TYPE_CALLBACK) {

		ompt_thread_type_callback_t new_func = (ompt_thread_type_callback_t)ompt_callback_list[event];
		omp_v_thread_t *t = __ompc_get_v_thread_by_num(__omp_myid);
		ompt_thread_type_t type = t->type;
		ompt_thread_id_t thread_id = __omp_myid + 1;
		(*new_func)(type, thread_id);

	} else if(which_event == WAIT_CALLBACK) {

		ompt_wait_callback_t new_func = (ompt_wait_callback_t)ompt_callback_list[event];
		(*new_func)(__ompc_get_v_thread_by_num(__omp_myid)->wait_id);

	} else if(which_event == PARALLEL_CALLBACK) {

		ompt_parallel_callback_t new_func = (ompt_parallel_callback_t)ompt_callback_list[event];
		omp_v_thread_t *t = __ompc_get_v_thread_by_num(__omp_myid);
		omp_team_t *team = t->team;
		ompt_task_id_t task_id = (__omp_current_task == NULL ? 0 : __omp_current_task->task_id);
		(*new_func)(__ompt_parallel_region_id, task_id);

	} else if(which_event == NEW_WORKSHARE_CALLBACK) {

		ompt_new_workshare_callback_t new_func = (ompt_new_workshare_callback_t)ompt_callback_list[event];
		omp_v_thread_t *t = __ompc_get_v_thread_by_num(__omp_myid);
		omp_team_t *team = t->team;
		ompt_task_id_t task_id = (t->implicit_task == NULL ? 0 : t->implicit_task->task_id);
		(*new_func)(team->parallel_region_id, task_id, t->entry_func);

	} else if(which_event == NEW_PARALLEL_CALLBACK) {

		ompt_new_parallel_callback_t new_func = (ompt_new_parallel_callback_t)ompt_callback_list[event];
		omp_v_thread_t *t = __ompc_get_v_thread_by_num(__omp_myid);
		omp_task_t *task = t->implicit_task;
		ompt_task_id_t parent_task_id = 0;
		omp_task_t *parent_task = NULL;
		ompt_frame_t *frame = NULL;
		if(task != NULL) {
			parent_task = task->parent;
			parent_task_id = parent_task == NULL ? 0 : parent_task->task_id;
			frame = &parent_task->frame_s;
		}
		(*new_func)(parent_task_id, frame, __ompt_parallel_region_id, __ompt_team_size, __ompt_entry_func);

	} else if(which_event == TASK_CALLBACK) {

		ompt_task_callback_t new_func = (ompt_task_callback_t)ompt_callback_list[event];
		ompt_task_id_t task_id = (__omp_current_task == NULL ? 0 : __omp_current_task->task_id);
		(*new_func)(task_id);

	} else if(which_event == TASK_SWITCH_CALLBACK) {

		ompt_task_switch_callback_t new_func = (ompt_task_switch_callback_t)ompt_callback_list[event];
		(*new_func)(__ompt_suspended_task_id, __ompt_resumed_task_id);

	} else if(which_event == NEW_TASK_CALLBACK) {

		ompt_new_task_callback_t new_func = (ompt_new_task_callback_t)ompt_callback_list[event];
		omp_v_thread_t *t = __ompc_get_v_thread_by_num(__omp_myid);
		ompt_task_id_t new_task_id = (t->implicit_task == NULL) ? 0 : t->implicit_task->task_id;
		omp_task_t *parent_task = __ompt_get_task_ancestor(1);
		ompt_frame_t *parent_frame = (parent_task != NULL ? &parent_task->frame_s : NULL);
		(*new_func)((parent_task == NULL) ? 0 : parent_task->task_id, parent_frame, new_task_id, t->entry_func);

	} else {
		(*ompt_callback_list[event])();
	}

}




/*
 * Public API Functions
 */

OMPT_API ompt_thread_id_t ompt_get_thread_id(void) {
	return __ompc_get_local_thread_num() + 1;
//	return __omp_myid + 1;
}


OMPT_API int ompt_set_callback( /* register a callback for an event */
		ompt_event_t event, /* the event of interest */
		ompt_callback_t callback /* function pointer for the callback */
) {
	if(event > ompt_event_flush)
		return 0;

	ompt_callback_list[event] = callback;
	return 1;
}


OMPT_API int ompt_get_callback( /* return the current callback for an event (if any) */
ompt_event_t event, /* the event of interest */
ompt_callback_t *callback /* pointer to receive the return value */
) {
	if(event > ompt_event_flush || ompt_callback_list[event] == NULL) {
		callback = NULL;
		return 0;
	}

	callback = ompt_callback_list[event];
	return 1;
}

OMPT_API ompt_parallel_id_t ompt_get_parallel_id( /* identify a parallel region */
int ancestor_level /* how many levels the ancestor is removed from the current region */
) {
	if (__omp_exe_mode & OMP_EXE_MODE_SEQUENTIAL)
		return 0;

	omp_v_thread_t *p_vthread = __ompc_get_v_thread_by_num( __omp_myid);
	if(p_vthread->state == ompt_state_idle)
		return 0;

	omp_team_t *team = __ompt_get_parallel_region_ancestor(ancestor_level);
	if(team == NULL)
		return 0;

	return team->parallel_region_id;
}

OMPT_API ompt_task_id_t *ompt_get_task_id( /* identify a task */
int depth /* how many levels removed from the current task */
) {
	omp_task_t *task = __ompt_get_task_ancestor(depth);
	return task->task_id;
}


OMPT_API ompt_state_t ompt_get_state( /* get the state for a thread */
ompt_wait_id_t *wait_id /* for wait states: identify what awaited */
) {
	if(wait_id != NULL) {
		*wait_id = __omp_current_v_thread->wait_id;
	}
//	return __omp_current_v_thread->executor->task->state;
//	return __ompc_get_current_v_thread()->state;
	return __omp_current_v_thread->state;
}

OMPT_API void * ompt_get_idle_frame( /* identify the idle frame (if any) for a thread */
void) {
	return __omp_current_v_thread->idle_frame;
}

OMPT_API ompt_frame_t *ompt_get_task_frame(
	int depth /* how many levels removed from the current task */
) {
	omp_task_t *task = __ompt_get_task_ancestor(depth);
	return task->frame_pointer;
}

OMPT_API int ompt_get_parallel_team_size( /* query # threads in a parallel region */
int ancestor_level /* how many levels the ancestor is removed from the current region */
) {
	omp_team_t *team = __ompt_get_parallel_region_ancestor(ancestor_level);
	if(team == NULL)
		return -1;
	return team->team_size;
}



#define NUMBER_OF_OMPT_STATE 17
ompt_state_t ompt_state_list[NUMBER_OF_OMPT_STATE] = {
		ompt_state_work_serial,
		ompt_state_work_parallel,
		ompt_state_work_reduction,
		ompt_state_idle,
		ompt_state_overhead,
		ompt_state_wait_barrier,
		ompt_state_wait_barrier_implicit,
		ompt_state_wait_barrier_explicit,
		ompt_state_wait_taskwait,
		ompt_state_wait_taskgroup,
		ompt_state_wait_lock,
		ompt_state_wait_nest_lock,
		ompt_state_wait_critical,
		ompt_state_wait_atomic,
		ompt_state_wait_ordered,
		ompt_state_undefined,
		ompt_state_first
};

char ompt_state_list_name[NUMBER_OF_OMPT_STATE][40] = {
		"ompt_state_work_serial",
		"ompt_state_work_parallel",
		"ompt_state_work_reduction",
		"ompt_state_idle",
		"ompt_state_overhead",
		"ompt_state_wait_barrier",
		"ompt_state_wait_barrier_implicit",
		"ompt_state_wait_barrier_explicit",
		"ompt_state_wait_taskwait",
		"ompt_state_wait_taskgroup",
		"ompt_state_wait_lock",
		"ompt_state_wait_nest_lock",
		"ompt_state_wait_critical",
		"ompt_state_wait_atomic",
		"ompt_state_wait_ordered",
		"ompt_state_undefined",
		"ompt_state_first"
};

OMPT_API int ompt_enumerate_state( /* extract the set of states supported */
ompt_state_t current_state, /* current state in the enumeration */
ompt_state_t *next_state, /* next state in the enumeration */
const char **next_state_name /* string description of next state */
) {

	int index = 0;
	// Finding state in the state_list
	for(index=0;index < NUMBER_OF_OMPT_STATE;index++)
		if(ompt_state_list[index] == current_state)
			break;
	index++;
	if(index >= NUMBER_OF_OMPT_STATE)
		index %= NUMBER_OF_OMPT_STATE;

	*next_state = ompt_state_list[index];
	*next_state_name = &ompt_state_list_name[index];

	if(*next_state == ompt_state_first)
		return 0;
	return 1;
}






/*
 *
 * OMPT public runtime functions
 *
 */

int ompt_initialize(
		ompt_function_lookup_t lookup, /* function to look up OMPT API routines by name */
		const char *runtime_version, /* OpenMP runtime version string */
		unsigned int ompt_version /* integer that identifies the OMPT revision */
) {

	return 0;
}


void ompt_control(uint64_t command, uint64_t modifier) {

	if(__ompt_is_initialized == 0)
		return;

	if(command == OMPT_CONTROL_START_RESTART) {
		__ompt_track_monitoring = 1;
	} else if(command == OMPT_CONTROL_PAUSE) {
		__ompt_track_monitoring = 0;
	} else if(command == OMPT_CONTROL_FLUSH_CONTINUE) {

	} else if(command == OMPT_CONTROL_PERMANENTLY_OFF) {
		__ompt_is_initialized = __ompt_track_monitoring = 0;
	}

	ompt_control_callback_t new_func = (ompt_control_callback_t)ompt_callback_list[ompt_event_control];
	(*new_func)(command, modifier);

}


ompt_interface_fn_t ompt_lookup(const char *interface_function_name) {

	if(strcmp(interface_function_name, "ompt_enumerate_state") == 0)
		return (ompt_interface_fn_t) ompt_enumerate_state;

	if(strcmp(interface_function_name, "ompt_get_thread_id") == 0)
		return (ompt_interface_fn_t) ompt_get_thread_id;

	if(strcmp(interface_function_name, "ompt_set_callback") == 0)
		return (ompt_interface_fn_t) ompt_set_callback;
	if(strcmp(interface_function_name, "ompt_get_callback") == 0)
		return (ompt_interface_fn_t) ompt_get_callback;

	if(strcmp(interface_function_name, "ompt_get_idle_frame") == 0)
		return (ompt_interface_fn_t) ompt_get_idle_frame;
	if(strcmp(interface_function_name, "ompt_get_state") == 0)
		return (ompt_interface_fn_t) ompt_get_state;

	if(strcmp(interface_function_name, "ompt_get_parallel_id") == 0)
		return (ompt_interface_fn_t) ompt_get_parallel_id;

	if(strcmp(interface_function_name, "ompt_get_task_frame") == 0)
		return (ompt_interface_fn_t) ompt_get_task_frame;
	if(strcmp(interface_function_name, "ompt_get_task_id") == 0)
		return (ompt_interface_fn_t) ompt_get_task_id;

	return NULL;
}
