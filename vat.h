/*
 * Copyright (c) 1991 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Lawrence Berkeley Laboratory,
 * Berkeley, CA.  The name of the University may not be used to
 * endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * @(#) $Header: /u/kon/hgs/src/cvsroot//rtptools/vat.h,v 1.1.1.1 1997/08/07 17:13:53 hgs Exp $ (LBL)
 * modified for NEVOT by H. Schulzrinne
 */

#ifndef vat_packet_h
#define vat_packet_h

/*
 * vat packet header (0 is MSB):
 *
 *  0                   1                   2                   3   
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | V |           |T|   |         |                               |
 * | e |    NSID   |S| 0 |  Audio  |        Conference ID          |
 * | r |           | |   | Format  |                               |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                                                               |
 * |                 Time Stamp (in audio samples)                 |
 * |                                                               |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *  protocol version (2 bits)
 *  no. of speaker id's (6 bits) [= 0 if src address should be used as id]
 *  flags (3 bits)
 *  audio format (5 bits)
 *  conference identifier (16 bits)
 *  timestamp [in audio samples] (32 bits)
 *  <0 to 63 4-byte speaker id's>
 *  <audio data>
 */
typedef struct {
	u_char nsid;
#define		NSID_MASK	0x3f
	u_char flags;
#define		VATHF_NEWTS	0x80	/* set if start of new talkspurt */
#define		VATHF_FMTMASK	0x1f	/* audio format bits */
	u_short confid;
	u_int ts;
} vat_hdr_t;

/*
 * Audio formats.
 *   Numbers for 'standard' encodings are assigned 0 upwards.
 *   Numbers for 'experimental' or non-standard encodings are
 *   assigned from 30 downwards.  31 is reserved.  The intent
 *   is that the lower numbers will be globally known and widely
 *   implemented & others will be used only between mutually
 *   consenting programs.
 */
typedef enum {
  VAT_AUDF_MULAW8 =  0, /* 64 kb/s 8KHz mu-law encoded PCM */
  VAT_AUDF_CELP   =  1, /* 4.8 kb/s FED-STD-1016 CELP */
  VAT_AUDF_G721   =  2, /* 32 kb/s CCITT ADPCM */
  VAT_AUDF_GSM    =  3, /* 13 kb/s GSM */
  VAT_AUDF_G723   =  4, /* 24 kb/s CCITT ADPCM */
  VAT_AUDF_L16_16 = 26, /* L16, 16000 samples/sec */
  VAT_AUDF_L16_44 = 27, /* L16, 44100 samples/sec, 2 channels */
  VAT_AUDF_LPC4   = 28, /* 4.8 kb/s LPC, 4 frames */
  VAT_AUDF_LPC1   = 29, /* 4.8 kb/s LPC, 1 frame */
  VAT_AUDF_IDVI   = 30, /* 32 kb/s Intel DVI ADPCM */
  VAT_AUDF_UNDEF  = 31  /* undefined */
} vat_audio_t;

/*
 * Structure of a vat control message (sent to the 'session' port).
 * All msgs have the same header:
 *  protocol version (2 bits)
 *  flags / misc (6 bits)
 *  msg type (8 bits)
 *  conference id
 *
 * An 'ID' message has a null-terminated ascii string giving the
 * site id string immediately following the header.
  *
 * A 'DONE' message has no other information.
 *
 * The structure of an IDLIST is described below.
 */
struct CtrlMsgHdr {
	u_char flags;
	u_char type;
	u_short confid;
};

/*
 * An IDLIST message contains:
 *  control msg hdr
 *  number of site id's in this msg (8 bits)
 *  audio flags (3 bits)
 *  audio format (5 bits)
 *  audio block size (16 bits)
 *  <site id entries>
 * Each site id entry consists of a 4 byte site id followed by a
 * null terminated ascii id string.  The string is padded with
 * nulls to the next 4 byte boundary.
 */
struct IDMsgHdr {
	struct CtrlMsgHdr h;
	u_char nsids;
	u_char ainfo;
	u_short blksize;
};
#endif
