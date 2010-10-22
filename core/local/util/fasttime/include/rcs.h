
/* $Id: rcs.h,v 1.1 2005/08/20 02:22:39 alexholkner Exp $ 
 *
 * Copyright (c) Internet2, 2005. All rights reserved.
 * See LICENSE file for conditions.
 */

/* rcs.h -- a way to put RCS IDs into executables.
 * Written by Stanislav Shalunov, http://www.internet2.edu/~shalunov/
 */

#ifndef RCS_H_INCLUDED
#define RCS_H_INCLUDED

#define RCS_ID(id) static const char *rcs_id = id;        \
static const char *                                \
        f_rcs_id(const char *s)                \
        {                                                \
                if (s) return s;                        \
                else return f_rcs_id(rcs_id);        \
        }

#endif /* ! RCS_H_INCLUDED */
