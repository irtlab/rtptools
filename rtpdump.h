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

typedef struct {
  struct timeval start;  /* start of recording (GMT) */
  u_int32 source;        /* network source (multicast address) */
  u_int16 port;          /* UDP port */
} RD_hdr_t;

typedef struct {
  u_int16 length;    /* length of packet, including this header (may 
                        be smaller than plen if not whole packet recorded) */
  u_int16 plen;      /* actual header+payload length for RTP, 0 for RTCP */
  u_int32 offset;    /* milliseconds since the start of recording */
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
