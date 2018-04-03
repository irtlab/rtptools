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

#include <stdint.h>
#include <stdio.h>
#include "payload.h"

struct pt payload[] = {
	{ "PCMU",	 8000,	1 },
	{ "reserved",	    0,	0 },
	{ "reserved",	    0,	0 },
	{ "GSM ",	 8000,	1 },
	{ "G723",	 8000,	1 },
	{ "DVI4",	 8000,	1 },
	{ "DVI4",	16000,	1 },
	{ "LPC ",	 8000,	1 },
	{ "PCMA",	 8000,	1 },
	{ "G722",	 8000,	1 },
	{ "L16 ",	44100,	2 },
	{ "L16 ",	44100,	1 },
	{ "QCELP",	 8000,	1 },
	{ "CN  ",	 8000,	0 },
	{ "MPA ",	90000,	0 },
	{ "G728",	 8000,	1 },
	{ "DVI4",	11025,	1 },
	{ "DVI4",	22050,	1 },
	{ "G729",	 8000,	1 },
	{ "reserved",	    0,	0 },
	{ "unassigned",	    0,	0 },
	{ "unassigned",	    0,	0 },
	{ "unassigned",	    0,	0 },
	{ "unassigned",	    0,	0 },
	{ "unassigned",	    0,	0 },
	{ "CelB",	90000,	0 },
	{ "JPEG",	90000,	0 },
	{ "unassigned",	    0,	0 },
	{ "nv  ",	90000,	0 },
	{ "unassigned",	    0,	0 },
	{ "unassigned",	    0,	0 },
	{ "H261",	90000,	0 },
	{ "MPV ",	90000,	0 },
	{ "MP2T",	90000,	0 },
	{ "H263",	90000,	0 },
	{ NULL,		    0,	0 }
};
