extern struct timeval *timer_set(struct timeval *interval, 
  Notify_func func, Notify_client client, int relative);
extern struct timeval *timer_get(struct timeval *timeout);
extern int timer_pending(void);
