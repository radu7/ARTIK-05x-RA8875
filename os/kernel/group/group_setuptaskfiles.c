/****************************************************************************
 *
 * Copyright 2016 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/
/****************************************************************************
 *  kernel/group/group_setuptaskfiles.c
 *
 *   Copyright (C) 2007-2008, 2010, 2012-2013 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <sched.h>
#include <errno.h>

#include <tinyara/fs/fs.h>
#include <tinyara/net/net.h>

#include "sched/sched.h"
#include "group/group.h"

#if CONFIG_NFILE_DESCRIPTORS > 0 || CONFIG_NSOCKET_DESCRIPTORS > 0

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Determine how many file descriptors to clone.  If CONFIG_FDCLONE_DISABLE
 * is set, no file descriptors will be cloned.  If CONFIG_FDCLONE_STDIO is
 * set, only the first three descriptors (stdin, stdout, and stderr) will
 * be cloned.  Otherwise all file descriptors will be cloned.
 */

#if defined(CONFIG_FDCLONE_STDIO) && CONFIG_NFILE_DESCRIPTORS > 3
#define NFDS_TOCLONE 3
#else
#define NFDS_TOCLONE CONFIG_NFILE_DESCRIPTORS
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sched_dupfiles
 *
 * Description:
 *   Duplicate parent task's file descriptors.
 *
 * Input Parameters:
 *   tcb - tcb of the new task.
 *
 * Return Value:
 *   None
 *
 ****************************************************************************/

#if CONFIG_NFILE_DESCRIPTORS > 0 && !defined(CONFIG_FDCLONE_DISABLE)
static inline void sched_dupfiles(FAR struct task_tcb_s *tcb)
{
	/* The parent task is the one at the head of the ready-to-run list */

	FAR struct tcb_s *rtcb = (FAR struct tcb_s *)g_readytorun.head;
	FAR struct file *parent;
	FAR struct file *child;
	int i;

	DEBUGASSERT(tcb && tcb->cmn.group && rtcb->group);

	/* Duplicate the file descriptors.  This will be either all of the
	 * file descriptors or just the first three (stdin, stdout, and stderr)
	 * if CONFIG_FDCLONE_STDIO is defined.  NFSDS_TOCLONE is set
	 * accordingly above.
	 */

	/* Get pointers to the parent and child task file lists */

	parent = rtcb->group->tg_filelist.fl_files;
	child = tcb->cmn.group->tg_filelist.fl_files;

	/* Check each file in the parent file list */

	for (i = 0; i < NFDS_TOCLONE; i++) {
		/* Check if this file is opened by the parent.  We can tell if
		 * if the file is open because it contain a reference to a non-NULL
		 * i-node structure.
		 */

		if (parent[i].f_inode) {
			/* Yes... duplicate it for the child */

			(void)file_dup2(&parent[i], &child[i]);
		}
	}
}
#else							/* CONFIG_NFILE_DESCRIPTORS && !CONFIG_FDCLONE_DISABLE */
#define sched_dupfiles(tcb)
#endif							/* CONFIG_NFILE_DESCRIPTORS && !CONFIG_FDCLONE_DISABLE */


/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: group_setuptaskfiles
 *
 * Description:
 *   Configure a newly allocated TCB so that it will inherit
 *   file descriptors and streams from the parent task.
 *
 * Parameters:
 *   tcb - tcb of the new task.
 *
 * Return Value:
 *   Zero (OK) is returned on success; A negated errno value is returned on
 *   failure.
 *
 * Assumptions:
 *
 ****************************************************************************/

int group_setuptaskfiles(FAR struct task_tcb_s *tcb)
{
#if CONFIG_NFILE_DESCRIPTORS > 0
	FAR struct task_group_s *group = tcb->cmn.group;

	DEBUGASSERT(group);
#endif
#ifndef CONFIG_DISABLE_PTHREAD
	DEBUGASSERT((tcb->cmn.flags & TCB_FLAG_TTYPE_MASK) != TCB_FLAG_TTYPE_PTHREAD);
#endif

#if CONFIG_NFILE_DESCRIPTORS > 0
	/* Initialize file descriptors for the TCB */

	files_initlist(&group->tg_filelist);
#endif

	/* Duplicate the parent task's file descriptors */

	sched_dupfiles(tcb);

	/* Allocate file/socket streams for the new TCB */

#if CONFIG_NFILE_STREAMS > 0
	return group_setupstreams(tcb);
#else
	return OK;
#endif
}

#endif							/* CONFIG_NFILE_DESCRIPTORS > 0 || CONFIG_NSOCKET_DESCRIPTORS > 0 */
