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

/*
* rtpdump file format
*
* The file starts with the tool to be used for playing this file,
* the multicast/unicast receive address and the port.
*
* #!rtpplay1.0 224.2.0.1/3456\n
*
* This is followed by one binary header (RD_hdr_t) and one RD_packet_t
* structure for each received packet.  All fields are in network byte
* order.  We don't need the source IP address since we can do mapping
* based on SSRC.  This saves (a little) space, avoids non-IPv4
* problems and privacy/security concerns. The header is followed by
* the RTP/RTCP header and (optionally) the actual payload.
*/
#include "sysdep.h"

typedef struct {
  struct timeval32 {
      uint32_t tv_sec;    /* start of recording (GMT) (seconds) */
      uint32_t tv_usec;   /* start of recording (GMT) (microseconds)*/
  } start;
  uint32_t source;        /* network source (multicast address) */
  uint16_t port;          /* UDP port */
  uint16_t padding;       /* padding */
} RD_hdr_t;

typedef struct {
  uint16_t length;   /* length of packet, including this header (may
                        be smaller than plen if not whole packet recorded) */
  uint16_t plen;     /* actual header+payload length for RTP, 0 for RTCP */
  uint32_t offset;   /* milliseconds since the start of recording */
} RD_packet_t;

typedef union {
  struct {
    RD_packet_t hdr;
    char data[8000];
  } p;
  char byte[8192];
} RD_buffer_t;

extern int RD_header(FILE *in, struct sockaddr_in *sin, int verbose);
extern int RD_read(FILE *in, RD_buffer_t *b);
