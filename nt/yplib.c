/*
 * modified by Akira T. 01/07/2002
 * changed the return value to error code for just in case
 */
#include "ypclnt.h"
 
int yp_get_default_domain(char **domain)
{
  return YPERR_YPERR;
}

int yp_match(char *domain, char *hosts, char *host,
             int hostlen, char **value, int *valuelen)
{
  return YPERR_YPERR;
}
