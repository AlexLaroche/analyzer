#include <stdlib.h>
#include <string.h>
#include <goblint.h>

struct s { int *p; int x; };

int main() {
  int g = 42;
  // memset returns dest; zeroing a fresh void* block gives the same zero blob as calloc.
  // This is APR's apr_pcalloc macro: memset(apr_palloc(p, size), 0, size).
  struct s *a = memset(malloc(sizeof(struct s)), 0, sizeof(struct s));
  __goblint_check(a->x == 0); // TODO: writing the zero blob over the malloc blob does not replace it
  __goblint_check(a->p == NULL); // TODO
  a->p = &g;
  __goblint_check(*(a->p) == 42);
  return 0;
}
