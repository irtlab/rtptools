#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdep.h"
#include "hsearch.h"

typedef struct entryb {
	ENTRY *entry;
	struct entryb *next;
} ENTRYB;

static ENTRYB **entries;
static size_t htsize;

/*
 * Gotten from GNU libc-2.0
 */
int searchIsPrime(unsigned candidate) {
  /* No even number and none less than 10 will be passed here.  */
  unsigned int divn = 3;
  unsigned int sq = divn * divn;

  while (sq < candidate && candidate % divn != 0)
    {
      ++divn;
      sq += 4 * divn;
      ++divn;
    }

  return candidate % divn != 0;
}

unsigned searchNextPrime(unsigned long seed) {
  /* Make it definitely odd.  */
  seed |= 1;

  while (!searchIsPrime (seed))
    seed += 2;

  return seed;
}

int hhash(char *key) {
	int r = 0;
	char *k = key;

	while (*k != '\0') {
		r += *k;
		k++;
	}

	return (r % htsize);
}

int hcreate(size_t mekments) {
	unsigned int i;

	htsize = searchNextPrime(mekments);
	entries = (ENTRYB **)malloc(htsize * sizeof(ENTRYB *));
	for (i=0; i<htsize; i++) {
		entries[i] = (ENTRYB *)malloc(sizeof(ENTRYB));
		entries[i]->entry = NULL;
		entries[i]->next = NULL;
	}

	return 0;
}

/*****************************************************/
/* There are two actions for hsearch, FIND and ENTER */
/* For FIND, we just compare the item.key, if we got */
/* the entry, we just return it. For ENTER, we need  */
/* to compare both the key and the data, if both the */
/* key and data are the same, we do not need to add  */
/* it in. For ENTER, the data can not be NULL, but   */
/* for FIND, the data can be NULL                    */
/*****************************************************/

ENTRY *hsearch(ENTRY item, ACTION action) {
	int r;
	ENTRYB *result;

	/* key should not be NULL */
  if (item.key == NULL)
    return NULL;

	r = hhash(item.key);

	result = entries[r];

  if (result->entry != NULL) {
    if (!strcmp(result->entry->key, item.key)) {
      if (action != ENTER)
        return (result->entry);
      else 
        if (result->entry->data != NULL && \
          item.data != NULL && \
          !strcmp(result->entry->data, item.data)) 
			    return (result->entry);
		}
	}
  
	if (action == ENTER) {
		if (result->entry != NULL)
			return result->entry;
		else {
			result->entry = (ENTRY *)malloc(sizeof(ENTRY));
			result->next = (ENTRYB *)malloc(sizeof(ENTRYB));
			result->next->entry = NULL;
			result->next->next = NULL;
			result->entry->key = strdup(item.key);
//			result->entry->data = strdup(item.data);
			result->entry->data = item.data;      /* fixed by Akira 12/27/01 */
			return result->entry;
		}
	} else {
		return result->entry;
	}
}

void hdestroy(void) {
	unsigned int i;
	ENTRYB *e, *p;

	for (i=0; i<htsize; i++) {
		e = entries[i];
		while (e->entry != NULL) {
			p = e;
			e = e->next;
			free(p->entry->key);
//			free(p->entry->data);               /* fixed by Akira 12/27/01 */
			free(p->entry);
			free(p);
		}
		free(e);
	}
}

/*
main() {
	ENTRY t, *r;

	hcreate(100);
	t.key = strdup("wxt");
	t.data = strdup("123455");
	r = hsearch(t, ENTER);
	t.key = strdup("fas");
	t.data = strdup("12das3455");
	r = hsearch(t, ENTER);
	t.key = strdup("asas");
	t.data = strdup("123fda455");
	r = hsearch(t, ENTER);
	t.key = strdup("dda");
	t.data = strdup("12dda3455");
	r = hsearch(t, ENTER);
	t.key = strdup("asas");
	t.data = strdup("123fda455");
	r = hsearch(t, FIND);
	printf("%s, %s\n", r->key, r->data);
	t.key = strdup("ddad");
	t.data = strdup("12dda3455");
	r = hsearch(t, FIND);
	if (r == NULL)
		printf("Not find.\n");
	hdestroy();
	return 0;
}
*/