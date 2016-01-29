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


#include "ompt_rtl.h"


extern ompt_callback_t ompt_callback_list[ompt_event_flush + 1];



int __ompt_get_ompt_env_var() {
	int omp_tool_status = OMP_TOOL_VAR_NOT_INITIALIZED;
	char *env_var_str = getenv("OMP_TOOL");
	if (env_var_str != NULL) {
		int env_var_val = strncasecmp(env_var_str, "enabled", 4);

		if (env_var_val == 0) {
		  omp_tool_status = OMP_TOOL_ENABLED;
		} else {
		  env_var_val = strncasecmp(env_var_str, "disabled", 4);
		  if (env_var_val == 0)  {
			  omp_tool_status = OMP_TOOL_DISABLED;
		  } else {
			  Not_Valid("OMP_TOOL should be set to: enabled/disabled");
		  }
		}
	}
	if(omp_tool_status == OMP_TOOL_VAR_NOT_INITIALIZED)
		omp_tool_status = OMP_TOOL_ENABLED;

	return omp_tool_status;
}

/* This constant is for number of paired events that should be checked
 * after initialization and before continuing with application. */
#define PAIRED_NUM 5
void __ompt_check_paired_begin_end() {

	// For future addition to this array,
	// just put the "begin" events here.
	// The next event is automatically considered as
	// the "end" event.
	int paired[PAIRED_NUM] = {
			ompt_event_idle_begin,
			ompt_event_wait_barrier_begin,
			ompt_event_wait_barrier_begin,
			ompt_event_wait_taskwait_begin,
			ompt_event_wait_taskgroup_begin
	};

	int i;
	for(i=0;i<PAIRED_NUM;i++) {
		int begin = paired[i];
		int end = begin + 1;
		if(ompt_callback_list[begin] != NULL && ompt_callback_list[end] == NULL) {
			char tmp[100];
			sprintf(tmp, "OMPT: Both callbacks in a pair should be implemented - Not implemented event (id = %d) ", end);
			Not_Valid(tmp);
		} else if(ompt_callback_list[begin] == NULL && ompt_callback_list[end] != NULL) {
			char tmp[100];
			sprintf(tmp, "OMPT: Both callbacks in a pair should be implemented - Not implemented event (id = %d) ", begin);
			Not_Valid(tmp);
		}
	}
}

