/********************************************************************
 *                                                                  *
 *  Voice Terminal (VT)                                             *
 *  June, 1991                                                      *
 *                                                                  *
 *  Written at USC/Information Sciences Institute from an earlier   *
 *  version developed by Bolt Beranek and Newman Inc.               *
 *                                                                  *
 *  Copyright (c) 1991 University of Southern California.           *
 *  All rights reserved.                                            *
 *                                                                  *
 *  Redistribution and use in source and binary forms are permitted *
 *  provided that the above copyright notice and this paragraph are *
 *  duplicated in all such forms and that any documentation,        *
 *  advertising materials, and other materials related to such      *
 *  distribution and use acknowledge that the software was          *
 *  developed by the University of Southern California, Information *
 *  Sciences Institute.  The name of the University may not be used *
 *  to endorse or promote products derived from this software       *
 *  without specific prior written permission.                      *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR    *
 *  IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED  *
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR      *
 *  PURPOSE.                                                        *
 *                                                                  *
 *******************************************************************/

/****************************************************************************/
/******                                                                ******/
/******                     Multiple Timer Package                     ******/
/******                                                                ******/
/****************************************************************************/
/*                                                                          */
/*      These routines manage an ordered queue of interval timers so        */
/*      that a single process may have multiple, independent timers         */
/*      pending.  Each timer is identified by an opaque client handle.      */
/*      These routines are intended to multiplex the timers onto the        */
/*      timeout mechanism of the select() call, or onto the single          */
/*      interval timer provided by setitimer().                             */
/*                                                                          */
/****************************************************************************/

#include "notify.h"      /* Notify_func */
#include <stdio.h>
#include <stdlib.h>      /* */
#include <sys/types.h>
#include <sys/time.h>    /* timeval, gettimeofday() */
#include <assert.h>
#include "sysdep.h"      /* system-dependent */

static char rcsid[] = "$Id: multimer.c,v 1.2 2002/09/01 13:16:12 hgs Exp $";

typedef struct TQE {
    struct TQE *link;           /* link to next timer */
    struct timeval time;        /* expiration time */
    struct timeval interval;    /* next interval */
    Notify_func func;           /* function to be invoked */
    Notify_client client;
    int  which;                 /* type; currently always ITIMER_REAL */
} TQE;

/* active timer queue, in time order */
static TQE *timerQ = (TQE *)0;

/* queue of free Timer Queue Elements */
static TQE *freeTQEQ = (TQE *)0;

#ifndef timeradd
void timeradd(struct timeval *a, struct timeval *b, 
  struct timeval *sum)
{
  sum->tv_usec = a->tv_usec + b->tv_usec;
  if (sum->tv_usec >= 1000000L) {       /* > to >=  by Akira 12/29/01 */
    sum->tv_sec = a->tv_sec + b->tv_sec + 1;
    sum->tv_usec -= 1000000L;
  }
  else {
    sum->tv_sec = a->tv_sec + b->tv_sec;
  }
} /* timeradd */
#endif

/*
* Return 1 if a < b, 0 otherwise.
*/
static int timerless(struct timeval *a, struct timeval *b)
{
  if (a->tv_sec < b->tv_sec ||
      (a->tv_sec == b->tv_sec && a->tv_usec < b->tv_usec)) return 1;
  return 0;
} /* timerless */

void timer_check(void)
{
  register struct TQE *np;

  for (np = timerQ; np; np = np->link) {
    assert(np->time.tv_usec < 1000000);
    assert(np->interval.tv_usec < 1000000);
  }
} /* timer_check */


/*
* This routine sets a timer event for the specified client.  The client
* pointer is opaque to this routine but must be unique among all clients.
* Each client may have only one timer pending.  If the interval specified
* is zero, the pending timer, if any, for this client will be cancelled.
* Otherwise, a timer event will be created for the requested amount of
* time in the future, and will be inserted in chronological order
* into the queue of all clients' timers.
* interval:  in: time interval
* func:      in: function to be called when time expires
* client:    in: first argument for the handler function
* relative:  in: flag; set relative to current time
*/
struct timeval *timer_set(struct timeval *interval, 
  Notify_func func, Notify_client client, int relative)
{
  register struct TQE *np, *op, *tp;    /* To scan the timer queue */

  /* scan the timer queue to see if client has pending timer */
  op = (struct TQE *)&timerQ;           /* Fudge OK since link is first */
  for (np = timerQ; np; op = np, np = np->link)
    if (np->client == client) {
      op->link = np->link;              /* Yes, remove the timer from Q */
      break;                            /*  and stop the search */
    }

  /*  if the requested interval is zero, just free the timer  */
  if (interval == 0) {
    if (np) {                   /* If we found a timer, */
      np->link = freeTQEQ;      /* link TQE at head of free Q */
      freeTQEQ = np;
    }
    return 0;                   /* return, no timer set */
  }

  /*  nonzero interval, calculate new expiration time  */
  if (!(tp = np)) {     /* If no previous timer, get a TQE */
    /* allocate timer */
    if (!freeTQEQ) {
      freeTQEQ = (TQE *)malloc(sizeof(TQE));
      freeTQEQ->link = (TQE *)0;
      freeTQEQ->interval.tv_usec = 0;
      freeTQEQ->interval.tv_sec  = 0;
    }
    tp = freeTQEQ;
    freeTQEQ = tp->link;
  }

  /* calculate expiration time */
  if (relative) {
    (void) gettimeofday(&(tp->time), (struct timezone *)0);
    timeradd(&(tp->time), interval, &(tp->time));
    assert(tp->time.tv_usec < 1000000);
  }
  else tp->time = *interval;
#ifdef DEBUG
  printf("timer_set(): %d.%06d\n", tp->time.tv_sec, tp->time.tv_usec);
#endif
  tp->func   = func;
  tp->client = client;
  tp->which  = ITIMER_REAL;

  /*  insert new timer into timer queue  */
  op = (struct TQE *)&timerQ;           /* fudge OK since link is first */
  for (np = timerQ; np; op = np, np=np->link) {
    if (timerless(&tp->time, &np->time)) break;
  }
  op->link = tp;    /* point prev TQE to new one */
  tp->link = np;    /* point new TQE to next one */

  timer_check(); /*DEBUG*/
  return &(tp->interval);
} /* timer_set */

/*
* This routine returns a timeout value suitable for use in a select() call.
* Before returning, all timer events that have expired are removed from the
* queue and processed.  If no timer events remain, a NULL pointer is returned
* so the select() will just block.  Otherwise, the supplied timeval struct is
* filled with the timeout interval until the next timer expires.

* Note:  This routine may be called recursively if the timer event handling
* routine leads to another select() call!  Therefore, we just take one timer
* at a time, and don't use static variables.
*/
struct timeval *timer_get(struct timeval *timeout)
{
  register struct TQE *tp;      /* to scan the timer queue */
  struct timeval now;           /* current time */

  timer_check(); /*DEBUG*/
  for (;;) {
    /* return null pointer if there is no timer pending. */
    if (!timerQ) return (struct timeval *)0;

    /* check head of timer queue to see if timer has expired */
    (void) gettimeofday(&now, (struct timezone *)0);
    if (timerless(&now, &timerQ->time)) { /* unexpired, calc timeout */
      timeout->tv_sec  = timerQ->time.tv_sec  - now.tv_sec;
      timeout->tv_usec = timerQ->time.tv_usec - now.tv_usec;
      if (timeout->tv_usec < 0) {
        timeout->tv_usec += 1000000L;
        --timeout->tv_sec;
      }
      assert(timeout->tv_usec < 1000000);
      return timeout;     /* timeout until timer expires */
    } else {              /* head timer has expired, */
      tp = timerQ;        /* so remove it from the */
      timerQ = tp->link;  /* timer queue, */
      tp->link = freeTQEQ;
      freeTQEQ = tp;
      /* restart timer (absolute) */
      if (tp->interval.tv_sec || tp->interval.tv_usec) {
        timeradd(&tp->interval, &tp->time, &tp->time);
        timer_set(&tp->time, tp->func, tp->client, 0);
      }
      (*(tp->func))(tp->client); /* call the event handler */
    }
  } /* loop to see if another timer expired */
} /* timer_get */


/*
* Return 1 if the timer queue is not empty.
*/
int timer_pending(void)
{
  return timerQ != 0; 
} /* timer_pending */
