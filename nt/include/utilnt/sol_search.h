/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1996, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#ifndef _SEARCH_H
#define	_SEARCH_H

#ifndef _SIZE_T
#define	_SIZE_T
typedef unsigned	size_t;
#endif

/* HSEARCH(3C) */
typedef enum { FIND, ENTER } ACTION;

typedef struct entry { char *key, *data; } ENTRY;

int hcreate(size_t);
void hdestroy(void);
ENTRY *hsearch(ENTRY, ACTION);

#endif	/* _SEARCH_H */
