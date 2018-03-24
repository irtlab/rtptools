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
* Declarations for ANSI compiler, missing from SunOS.
*/
#if sun
/* ctype.h */
extern int tolower(int);
extern int toupper(int);

/* curses.h */
#ifdef WINDOW
extern int endwin();
extern int waddch();
extern int waddstr();
extern int wclear();
extern int wclrtoeol();
extern int wgetch(WINDOW *win);
extern int wmove();
extern int wrefresh();
extern int wstandend();
extern int wstandout();
#endif

/* <malloc.h> */

/* <string.h> */
#ifdef __string_h
extern int strncasecmp(const char *s1, const char *s2, size_t n);
extern int strcasecmp(const char *s1, const char *s2);
extern char *strerror(int);
#endif

/* <stdio.h> */
#ifdef FILE
extern int fclose(FILE *stream);
extern int fprintf(FILE *stream, char *, ...);
extern char *sprintf();  /* gcc also defines this... */
extern int printf(char *fmt, ...);
extern int sscanf(char *s, char *fmt, ...);
extern int fgetc(FILE *stream);
extern int _flsbuf();
extern int _filbuf();
extern int fread(char *ptr, int size, int nitems, FILE *stream);
extern int fseek(FILE *stream, long offset, int ptrname);
extern int puts(char *s);
extern int fputs(char *s, FILE *stream);
extern int fputc(char, FILE *stream);
extern int fwrite(char *ptr, int size, int nitems, FILE *stream);
extern int fflush(FILE *stream);
extern int setlinebuf(FILE *stream);
extern void setbuf(FILE *stream, char *buf);
#if defined(_sys_varargs_h) || defined(_STDARG_H) || defined(_ANSI_STDARG_H)
extern char *vsprintf(char *s, char *format, va_list ap);
#endif
#endif

/* stdlib.h */
#ifdef __stdlib_h
extern int getopt(int argc, char **argv, char *optstring);
extern void perror(char *s);
extern int qsort(void *base, long unsigned nel, long unsigned int
  width, int (*compar)(const void *, const void *));
extern long strtol(char *str, char **ptr, int base);
extern int atoi(const char *str);
extern int system(char *cmd);
#endif

/* search.h */
#ifdef _search_h
extern int hcreate(int nel);
extern ENTRY *hsearch(ENTRY item, ACTION action);
#endif

/* sys/socket.h */
#ifdef _sys_socket_h
extern int accept(int, void *, int *);
extern int bind(int, const void *, int);
extern int connect(int, const void *, int);
extern int getpeername(int, void *, int *);
extern int getsockname(int, void *, int *);
extern int listen(int, int);
extern int recvfrom(int, void *, int, int, void *, int *);
extern int sendmsg(int, const struct msghdr *, int);
extern int sendto(int, const void *, int, int, const void *, int);
extern int setsockopt(int, int, int, const void *, int);
extern int socket(int, int, int);
#endif

/* time.h */
#ifdef __time_h
extern time_t time(time_t *);
extern clock_t clock(void);
extern int strftime(char *buf, int bufsize, char *fmt, struct tm *tm);
#endif

/* sgtty.h */
#ifdef _sgtty_h
extern int stty(int fd, struct sgttyb *buf);
extern int gtty(int fd, struct sgttyb *buf);
#endif


#endif /* sun */

/*** SGI ***/
#ifdef sgi

#ifdef __RPCSVC_YPCLNT_H__
extern int yp_get_default_domain(char **outdomain);
#endif
#endif /* sgi */

/*** HP ***/
#ifdef hp

#endif /* hp */

