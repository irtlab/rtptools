#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>  /* struct sockaddr */
#include <netinet/in.h>
#include <arpa/inet.h>   /* inet_ntoa() */
#include <string.h>      /* strncmp() */
#include <sys/time.h>
#include <time.h>        /* localtime() added by Akira 12/27/01 */
#include "types.h"
#include "rtpdump.h"
#define RTPFILE_VERSION "1.0"

/*
* Read header. Return -1 if not valid, 0 if ok.
*/
int RD_header(FILE *in, struct sockaddr_in *sin, int verbose)
{
  RD_hdr_t hdr;
  char line[80], magic[80];

  if (fgets(line, sizeof(line), in) == NULL) return -1;
  sprintf(magic, "#!rtpplay%s ", RTPFILE_VERSION);
  if (strncmp(line, magic, strlen(magic)) != 0) return -1;
  if (fread((char *)&hdr, sizeof(hdr), 1, in) == 0) return -1;
  hdr.start.tv_sec = ntohl(hdr.start.tv_sec);
  hdr.port         = ntohs(hdr.port);
  if (verbose) {
    struct tm *tm;
    struct in_addr in;

    in.s_addr = hdr.source;
    tm = localtime(&hdr.start.tv_sec);
    strftime(line, sizeof(line), "%C", tm);
    printf("Start:  %s\n", line);
    printf("Source: %s (%d)\n", inet_ntoa(in), hdr.port);
  }
  if (sin && sin->sin_addr.s_addr == 0) {
    sin->sin_addr.s_addr = hdr.source;
    sin->sin_port        = htons(hdr.port);
  }
  return 0;
} /* RD_header */


/*
* Read next record from input file.
*/
int RD_read(FILE *in, RD_buffer_t *b)
{
  /* read packet header from file */
  if (fread((char *)b->byte, sizeof(b->p.hdr), 1, in) == 0) {
    /* we are done */
    return 0;
  }

  /* convert to host byte order */
  b->p.hdr.length = ntohs(b->p.hdr.length) - sizeof(b->p.hdr);
  b->p.hdr.offset = ntohl(b->p.hdr.offset);
  b->p.hdr.plen   = ntohs(b->p.hdr.plen);

  /* read actual packet */
  if (fread(b->p.data, b->p.hdr.length, 1, in) == 0) {
    perror("fread body");
  } 
  return b->p.hdr.length; 
} /* RD_read */
