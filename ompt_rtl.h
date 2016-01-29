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


#ifndef __OMPT_RTL_H_
#define __OMPT_RTL_H_

#include "omp_collector_api.h"
#include "ompt.h"
#include "omp_rtl.h"
//#include "omp_thread.h"

#ifdef OMPT
#ifndef USE_COLLECTOR_TASK
#define USE_COLLECTOR_TASK
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Global Variable Definitions
 */

extern volatile int __ompt_is_initialized;
extern volatile int __ompt_track_monitoring;

ompt_task_id_t __ompt_suspended_task_id;
ompt_task_id_t __ompt_resumed_task_id;


/*
 * Enumerations
 */

enum {
	OMP_TOOL_VAR_NOT_INITIALIZED,
	OMP_TOOL_DISABLED,
	OMP_TOOL_ENABLED
};


/*
 * Global Functions
 */

extern void __ompc_event_callback(OMP_COLLECTORAPI_EVENT event);
extern void __ompc_set_state(OMP_COLLECTOR_API_THR_STATE state);



/*
 * Internal API Functions
 */

int __ompt_get_ompt_env_var();
void __ompt_check_paired_begin_end();
void __ompt_call_initialized_callbacks(int threads_to_create);

/*
 * Inline Functions
 */

/* For internal use only */
inline void __ompt_set_state(ompt_state_t state, ompt_wait_id_t wait_id) {
#ifdef OMPT
	omp_v_thread_t *p_vthread = __ompc_get_v_thread_by_num( __omp_myid);
//	omp_v_thread_t *p_vthread = __ompc_get_current_v_thread();
	p_vthread->state = state;
	p_vthread->wait_id = wait_id;
#endif
}
/* For internal use only */
inline int __ompt_get_state() {
#ifdef OMPT
	omp_v_thread_t *p_vthread = __ompc_get_v_thread_by_num( __omp_myid);
//	omp_v_thread_t *p_vthread = __ompc_get_current_v_thread();
	return p_vthread->state;
#else
	return 0;
#endif
}


inline void __ompc_ompt_set_state(OMP_COLLECTOR_API_THR_STATE state_ompc, ompt_state_t state_ompt, ompt_wait_id_t wait_id) {
#ifdef OMPT
      __ompt_set_state(state_ompt, wait_id);
#else
      __ompc_set_state(state_ompc);
#endif
}

inline void __ompc_ompt_event_callback(OMP_COLLECTORAPI_EVENT event_ompc, ompt_event_t event_ompt) {
#ifdef OMPT
      __ompt_event_callback(event_ompt);
#else
      __ompc_event_callback(event_ompc);
#endif
}



#ifdef __cplusplus
}
#endif

#endif
