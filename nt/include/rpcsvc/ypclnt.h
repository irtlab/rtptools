#ifndef _RPCSVC_YPCLNT_H
#define _RPCSVC_YPCLNT_H

extern int yp_get_default_domain(char **);
extern int yp_match(char *, char *, char *, int, char **, int *);

#endif