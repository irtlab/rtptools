/*
notify  --  primitive notification service implementing a subset of the SunOS
            (sunview/XView) notifier.

Copyright 1993 by AT&T Bell Laboratories; all rights reserved
*/

#include <sys/time.h>
#include <signal.h>
#include <stdio.h>     /*DEBUG, fprintf() */
#include <stdlib.h>    /* malloc() */
#include <unistd.h>    /* select() */
#include "sysdep.h"
#include "notify.h"
#include "multimer.h"
#include "ansi.h"

static char rcsid[] = "$Id$";

#ifdef hp
#define CAST int *
#else
#define CAST fd_set *
#endif

typedef struct event_t {
  struct event_t *next;
  Notify_client client;
  Notify_func_input func;
  int fd;  
  enum type_t {N_input, N_itimer} type;
} event_t;

static event_t *el;   /* event list */
static int max_fd;    /* highest file descriptor used */
static fd_set Readfds, Writefds, Exceptfds;
static int stop;

/* signal list */
static struct {
    Notify_client client;
    Notify_func_signal signal_func;
    Notify_signal_mode when;
} s[NSIG];


static event_t *search(
  Notify_client client,  /* ignored if fd >= 0 */
  int fd, 
  enum type_t type,
  event_t **prev         /* element before target */
)
{
  event_t *e;

  *prev = 0;
  for (e = el; e; *prev = e, e = e->next) {
    if (e->type == type && e->fd == fd && (fd >= 0 || e->client == client))
      return e;
  }
  return 0;
} /* search */


/*
* Find maximum file descriptor number used.
*/
static void set_max_fd(void)
{
  event_t *e;

  max_fd = 0;
  for (e = el; e; e = e->next) {
    if (e->fd > max_fd) max_fd = e->fd;
  }
} /* set_max */


/*
* Install input handler function 'func' for file descriptor 'fd'.
* func=NOTIFY_FUNC_NULL removes handler.
*/
Notify_func_input notify_set_input_func(
  Notify_client client,   /* argument passed to function */
  Notify_func_input func, /* function to be called: func(client, fd) */
  int fd)                 /* file descriptor */
{
  event_t *e, *prev;

  e = search(client, fd, N_input, &prev);
  if (!e) {  /* create new event */
    if (func == NOTIFY_FUNC_INPUT_NULL) return func;
    e = (event_t *)malloc(sizeof(event_t));
    if (!e) return 0;
    e->next = el;   /* put at head of list */
    el = e;
    if (fd > max_fd) max_fd = fd;
    e->type   = N_input;
    e->client = client;
    e->func   = func;
    e->fd     = fd;
    FD_SET(fd, &Readfds);
  }
  else {
    if (func == NOTIFY_FUNC_INPUT_NULL) {
      FD_CLR(fd, &Readfds);
      if (prev) prev->next = e->next; 
      else el = e->next;
      free(e);
      set_max_fd();  /* find new maximum fd */
      return func;
    }
    else e->func = func;
  }
  return 0;
} /* notify_set_input_func */


/*
* Interval timer initialization.
*/
Notify_func notify_set_itimer_func(
  Notify_client client,      /* value passed to function */
  Notify_func timer_func,    /* function to be called */
  int which,                 /* not used */
  struct itimerval *value,   /* interval */
  struct itimerval *ovalue)  /* not used */
{
  struct timeval *t;

  /* set relative to now */
  t = timer_set(value ? &(value->it_value) : 0, timer_func, client, 1);
  *t = value->it_interval;
  return 0;   /* kludge */
} /* notify_set_itimer_func */


/*
* Don't wait if there are no other events.
*/
static struct timeval *timer_get_pending(struct timeval *timeout, int max_fd) {
  struct timeval *tvp;

  tvp = timer_get(timeout);
  if (!tvp && !max_fd) {
    timeout->tv_sec = timeout->tv_usec = 0;
    notify_stop();
  }
  return timeout;
} /* timer_get_pending */


/*
* Main loop. Return 0 if stopped, -1 if error.
*/
Notify_error notify_start(void)
{
  struct timeval timeout;
  event_t *e, *prev;
  int fd;
  int found;
  fd_set readfds, writefds, exceptfds;

  stop = 0;
  while (!stop) {
    readfds   = Readfds;
    writefds  = Writefds;
    exceptfds = Exceptfds;
    timeout.tv_sec  = 0;
    timeout.tv_usec = 0;

    found = select(max_fd+1, (CAST)&readfds, (CAST)&writefds, (CAST)&exceptfds, 
      timer_get_pending(&timeout, max_fd));

    /* found = 0: just a timer -> do nothing */
    if (found < 0) {
      fprintf(stderr, "Timeout: %lu.%06lu\n", timeout.tv_sec, timeout.tv_usec);
      return -1;
    }
    for (fd = 0; fd <= max_fd && found > 0; fd++) {
      if (FD_ISSET(fd, &readfds)) {
        e = search((Notify_client)0, fd, N_input, &prev);
        if (e) {
          if (e->func) (e->func)(e->client, fd);
        }
        else {
          fprintf(stderr, "No handler for fd %d\n", fd);
        }
      }
    };
  }
  return 0;
} /* notify_start */


/*
* Stop the event loop. Noticed only at next event.
*/
Notify_error notify_stop(void)
{
  stop = 1;
  return 0;  /* kludge */
} /* notify_stop */


/*
* Actually invoked by signal(). Calls user-defined handler.
*/
static void sig_handler(int sig)
{
  (*s[sig].signal_func)(s[sig].client, sig, s[sig].when);
} /* sig_handler */


/*
* Install signal handler.
*/
Notify_func_signal notify_set_signal_func(Notify_client client, 
  Notify_func_signal signal_func, int sig, Notify_signal_mode when)
{
  Notify_func_signal old_func = s[sig].signal_func;
  s[sig].signal_func = signal_func;
  s[sig].when        = when;
  s[sig].client      = client;

  signal(sig, sig_handler);
  return old_func;
} /* notify_set_signal_func */


/*
 * Sets the value of fd_sets Rreadfds, Writefds, Exceptfds.
 */
void notify_set_socket(int sock, int flag)
{
  switch (flag) {
  case 0:
    FD_SET(sock, &Readfds);
    break;
  case 1:
    FD_SET(sock, &Writefds);
    break;
  case 2:
    FD_SET(sock, &Exceptfds);
    break;
  default:
    break;
  }
} /* notify_set_socket */
