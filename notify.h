/*
stripped-down version of <xview/notify.h>

(c) Copyright 1989, 1990, 1991 Sun Microsystems, Inc. Sun design
patents pending in the U.S. and foreign countries. OPEN LOOK is
a trademark of USL. Used by written permission of the owners.
*/
#ifndef _notify_h
#define _notify_h
#include <sys/types.h>
#include <sys/time.h>

/*
 * Client notification function return values for notifier to client calls.
 */
typedef enum notify_value {
  NOTIFY_DONE		= 0,	/* Handled notification */
  NOTIFY_IGNORED	= 1,	/* Did nothing about notification */
  NOTIFY_UNEXPECTED	= 2	    /* Notification not expected */
} Notify_value;

typedef enum notify_error {
  NOTIFY_OK               = 0,    /* Success */
  NOTIFY_UNKNOWN_CLIENT   = 1,    /* Client argument unknown to notifier */
  NOTIFY_NO_CONDITION     = 2,    /* Client not registered for given
                                   * condition
                                   */
  NOTIFY_BAD_ITIMER       = 3,    /* Itimer type unknown */
  NOTIFY_BAD_SIGNAL       = 4,    /* Signal number out of range */
  NOTIFY_NOT_STARTED      = 5,    /* Notify_stop called & notifier not
                                   * started
                                   */
  NOTIFY_DESTROY_VETOED   = 6,    /* Some client didn't want to die when
                                   * called notify_die(DESTROY_CHECKING)
                                   */
  NOTIFY_INTERNAL_ERROR   = 7,    /* Something wrong in the notifier */
  NOTIFY_SRCH             = 8,    /* No such process */
  NOTIFY_BADF             = 9,    /* Bad file number */
  NOTIFY_NOMEM            = 10,   /* Not enough core */
  NOTIFY_INVAL            = 11,   /* Invalid argument */
  NOTIFY_FUNC_LIMIT       = 12    /* Too many interposition functions */
} Notify_error;

typedef enum notify_signal_mode {
  NOTIFY_SYNC             = 0,
  NOTIFY_ASYNC            = 1
} Notify_signal_mode;

/*
 * Opaque client handle.
 */
typedef	unsigned long Notify_client;

/*
 * A pointer to functions returning a Notify_value.
 */
typedef	Notify_value (*Notify_func)(Notify_client client);
typedef	Notify_value (*Notify_func_input)(Notify_client client, int fd);
typedef	Notify_value (*Notify_func_signal)(Notify_client client, int
  signal, Notify_signal_mode mode);

#define	NOTIFY_FUNC_NULL        ((Notify_func)0)
#define	NOTIFY_FUNC_INPUT_NULL  ((Notify_func_input)0)
#define	NOTIFY_FUNC_SIGNAL_NULL ((Notify_func_signal)0)

/*
 * Establish event handler for file descriptor 'fd'. The handler
 * 'func' is called whenever 'fd' is ready for input. Calling
 * notify_set_input_func with func=NOTIFY_FUNC_NULL removes
 * the handler. Return 'func' on success, NOTIFY_FUNC_NULL
 * on failure.
 */
extern Notify_func_input notify_set_input_func (Notify_client nclient,
  Notify_func_input func, int fd);

/*
 * Establish event handler for file descriptor 'fd'. The handler
 * 'func' is called whenever 'fd' is ready for output. Calling
 * notify_set_input_func with func=NOTIFY_FUNC_NULL removes
 * the handler. Return 'func' on success, NOTIFY_FUNC_NULL
 * on failure.
 */
extern Notify_func notify_set_output_func (Notify_client nclient,
  Notify_func func, int fd);

/*
 * Establish event handler that is called periodically. 
 * The function 'func' is called every 'interval' milliseconds.
 * To change the period, call the function with the original
 * 'client' and 'func' values, and a changed value of 'interval'.
 * An interval value of 0 disables the periodic invocation of 
 * 'func'. No return value.
 */
extern void notify_set_periodic_func(Notify_client client, Notify_func
  func, int interval);

/*
* Main loop. Never exits unless notify_stop() is called.
*/
extern  Notify_error  notify_start(void);

/*
* Abort event handler.
*/
extern  Notify_error  notify_stop(void);

/*
* Establish signal handler.
*/
extern Notify_func_signal notify_set_signal_func(Notify_client nclient,
  Notify_func_signal func, int sig, Notify_signal_mode mode);

/*
* Initialize Readfds (flag = 0), Writefds (1), Exceptfds (2).
*/
extern void notify_set_socket(int sock, int flag);

#endif
