/*
 * Copyright (c) 2018 Jan Stary <hans@stare.cz>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/time.h>

int
main(void)
{
	struct timeval a, b, c;

	if (ITIMER_REAL != 0)
		return 1;

	a.tv_sec  = 4;
	a.tv_usec = 3;
	b.tv_sec  = 2;
	b.tv_usec = 1;

	timeradd(&a, &b, &c);
	if ((c.tv_sec != 6) || (c.tv_usec != 4))
		return 1;

	timersub(&a, &b, &c);
	if ((c.tv_sec != 2) || (c.tv_usec != 2))
		return 1;

	return 0;
}
