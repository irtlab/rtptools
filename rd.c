/*
 * (c) 1998-2018 by Columbia University; all rights reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#ifndef WIN32
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "rtpdump.h"

#define RTPFILE_VERSION "1.0"

/*
* Read header. Return -1 if not valid, 0 if ok.
*/
int RD_header(FILE *in, struct sockaddr_in *sin, int verbose)
{
  RD_hdr_t hdr;
  time_t tt;
  char line[80], magic[80];

  if (fgets(line, sizeof(line), in) == NULL) return -1;
  sprintf(magic, "#!rtpplay%s ", RTPFILE_VERSION);
  if (strncmp(line, magic, strlen(magic)) != 0) return -1;
  if (fread((char *)&hdr, sizeof(hdr), 1, in) == 0) return -1;
  hdr.start.tv_sec = ntohl(hdr.start.tv_sec);
  if (verbose) {
    struct tm *tm;
    struct in_addr in;

    in.s_addr = hdr.source;
    tt = (time_t)(hdr.start.tv_sec);
    tm = localtime(&tt);
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
