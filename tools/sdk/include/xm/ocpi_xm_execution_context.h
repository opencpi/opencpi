
/*
 * Copyright (c) 2009 and 2010 Mercury Federal Systems.
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_OCPI_XM_EXECUTION_CONTEXT_H
#define INCLUDED_OCPI_XM_EXECUTION_CONTEXT_H

#ifdef __cplusplus
  extern "C" {
#endif

#include <ucontext.h>

#define XM_TODO_INDEX ( 7 )

typedef struct
{
  double count;
  int done;
} XmTodoState;

#define XM_CONTEXT_INDEX ( 0 )

typedef struct
{
  ucontext_t caller;
  ucontext_t primitive;
  int start_is_done;
  int run_is_done;
  int stop_is_done;
  int finish_is_done;
  char stack_primitive [ SIGSTKSZ ];

} ExecutionContext;

#ifdef __cplusplus
  }
#endif

#endif // End: #ifndef INCLUDED_OCPI_XM_EXECUTION_CONTEXT_H
