/*
 *  Warning eater for compiling win32
 *  by Akira T. 01/07/2002
 */

#define YPERR_YPERR     6            /* Internal yp server or client error */

extern int yp_get_default_domain(char **domain);

extern int yp_match(char *domain, char *hosts, char *host,
                    int hostlen, char **value, int *valuelen);

