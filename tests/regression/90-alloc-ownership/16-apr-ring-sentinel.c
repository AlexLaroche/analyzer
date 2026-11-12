// PARAM: --set ana.malloc.unique_address_count 1
// APR rings: writes through the sentinel (a fake element overlaying the ring head)
// must land on the head's next/prev fields, so an inserted element is found again.
#include <stdlib.h>
#include <goblint.h>
#include "apr_ring_min.h"

typedef struct prod prod;
struct prod {
  int state;
  APR_RING_ENTRY(prod) link; // not the first field: sentinel = &head.next - 8
  int (*fn)(void);
};
APR_RING_HEAD(prod_ring, prod);

struct prod_ring active;

int f(void) { return 42; }

int main(void) {
  APR_RING_INIT(&active, prod, link);
  __goblint_check(APR_RING_EMPTY(&active, prod, link));

  prod *p = malloc(sizeof(prod));
  p->fn = f;
  APR_RING_INSERT_TAIL(&active, p, prod, link);
  __goblint_check(!APR_RING_EMPTY(&active, prod, link));
  __goblint_check(APR_RING_FIRST(&active) == p);
  __goblint_check(APR_RING_NEXT(p, link) == APR_RING_SENTINEL(&active, prod, link));

  prod *q = APR_RING_FIRST(&active);
  int r = q->fn();
  __goblint_check(r == 42);

  APR_RING_REMOVE(q, link);
  __goblint_check(APR_RING_EMPTY(&active, prod, link));
  return 0;
}
